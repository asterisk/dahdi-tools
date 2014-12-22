#define	_GNU_SOURCE	/* for memrchr() */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <xtalk/debug.h>
#include <autoconfig.h>
#include "xusb_common.h"

#define	DBG_MASK	0x01

const char *xusb_tt_name(enum xusb_transfer_type tt)
{
	switch (tt) {
	case XUSB_TT_BULK: return "BULK";
	case XUSB_TT_INTERRUPT: return "INTERRUPT";
	case XUSB_TT_ILLEGAL:
		break;
	}
	return "ILLEGAL";
}

/* GCC versions before 4.6 did not support neither push and pop on
 * the diagnostic pragma nor applying it inside a function.
 */
#ifndef HAVE_GCC_PRAGMA_DIAG_STACK
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
int xusb_printf(const struct xusb_iface *iface, int level, int debug_mask,
	const char *prefix, const char *fmt, ...)
{
	int n;
	va_list ap;
	char fmtbuf[BUFSIZ];
	char tmpbuf[BUFSIZ];

	snprintf(fmtbuf, sizeof(fmtbuf), "%s%03d/%03d[%d] %s",
		prefix,
		xusb_bus_num(iface->xusb_device),
		xusb_device_num(iface->xusb_device),
		xusb_interface_num(iface),
		fmt);
	va_start(ap, fmt);
	n = vsnprintf(tmpbuf, sizeof(tmpbuf), fmtbuf, ap);
	va_end(ap);
#ifdef HAVE_GCC_PRAGMA_DIAG_STACK
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
	log_function(level, debug_mask, tmpbuf);
#ifdef HAVE_GCC_PRAGMA_DIAG_STACK
#pragma GCC diagnostic pop
#endif
	return n;
}
#ifndef HAVE_GCC_PRAGMA_DIAG_STACK
#pragma GCC diagnostic error "-Wformat-security"
#endif

int xusb_printf_details(const struct xusb_iface *iface, int level, int debug_mask,
	const char *file, int line, const char *severity, const char *func,
	const char *fmt, ...)
{
	int n;
	va_list ap;
	char prefix[BUFSIZ];
	char tmpbuf[BUFSIZ];

	va_start(ap, fmt);
	vsnprintf(tmpbuf, sizeof(tmpbuf), fmt, ap);
	va_end(ap);
	snprintf(prefix, sizeof(prefix), "%s:%d: %s(%s): ",
		file, line, severity, func);
	va_start(ap, fmt);
	n = xusb_printf(iface, level, DBG_MASK, prefix, tmpbuf);
	va_end(ap);
	return n;
}

void xusb_init_spec(struct xusb_spec *spec, char *name,
	uint16_t vendor_id, uint16_t product_id)
{
	DBG("Initialize [%02X:%02X] - %s\n", vendor_id, product_id, name);
	memset(spec, 0, sizeof(*spec));
	spec->name = name;
	spec->vendor_id = vendor_id;
	spec->product_id = product_id;
}

const struct xusb_spec *xusb_device_spec(const struct xusb_device *xusb_device)
{
	return xusb_device->spec;
}

/*
 * Match the string "tail" as the tail of string "path"
 * Returns 1 in case they match, 0 otherwise
 */
int match_devpath(const char *path, const char *tail)
{
	int len_path = strlen(path);
	int len_tail = strlen(tail);
	int path_offset = len_path - len_tail;

	if (path_offset < 0)
		return 0;
	return strstr(path + path_offset, tail) != NULL;
}

int match_device(const struct xusb_device *xusb_device,
		const struct xusb_spec *spec)
{
	assert(xusb_device);
	DBG("Checking: %04X:%04X: "
			"\"%s\"\n",
			spec->vendor_id,
			spec->product_id,
			spec->name);
	if (xusb_device->idVendor != spec->vendor_id) {
		DBG("Wrong vendor id 0x%X\n", xusb_device->idVendor);
		return 0;
	}
	if (xusb_device->idProduct != spec->product_id) {
		DBG("Wrong product id 0x%X\n", xusb_device->idProduct);
		return 0;
	}
	return	1;
}

struct xusb_device *xusb_deviceof(struct xusb_iface *iface)
{
	return iface->xusb_device;
}

int xusb_is_claimed(struct xusb_iface *iface)
{
	return iface->is_claimed != 0;
}

struct xusb_iface *xusb_interface_of(const struct xusb_device *dev, int num)
{
	return dev->interfaces[num];
}

#define	XUSB_IFACE_DUMP(prefix, level, iface) \
	XUSB_PRINT((iface), level, "%s%d\tep_out=0x%2X ep_in=0x%02X [%s]\n", \
			(prefix), \
			(iface)->interface_num, \
			(iface)->ep_out, \
			(iface)->ep_in, \
			(iface)->iInterface)

void xusb_list_dump(struct xlist_node *xusb_list)
{
	struct xlist_node	*curr;
	struct xusb_device	*xusb_device;

	for (curr = xusb_list->next; curr != xusb_list; curr = curr->next) {
		struct xusb_iface **piface;

		xusb_device = curr->data;
		assert(xusb_device);
		DBG("%s: usb:ID=%04X:%04X [%s / %s / %s]\n",
			xusb_device->devpath_tail,
			xusb_device->idVendor,
			xusb_device->idProduct,
			xusb_device->iManufacturer,
			xusb_device->iProduct,
			xusb_device->iSerialNumber
			);
		for (piface = xusb_device->interfaces; *piface; piface++)
			XUSB_IFACE_DUMP("\t", DEBUG, *piface);
	}
}

void xusb_destroy_interface(struct xusb_iface *iface)
{
	if (iface) {
		xusb_release(iface);
		XUSB_DBG(iface, "MEM: FREE interface\n");
		memset(iface, 0, sizeof(*iface));
		free(iface);
		iface = NULL;
	}
}

static const char *path_tail(const char *path)
{
	const char	*p;

	assert(path != NULL);
	/* Find last '/' */
	p = memrchr(path, '/', strlen(path));
	if (!p) {
		ERR("Missing a '/' in %s\n", path);
		return NULL;
	}
	/* Search for a '/' before that */
	p = memrchr(path, '/', p - path);
	if (!p)
		p = path;		/* No more '/' */
	else
		p++;			/* skip '/' */
	return p;
}

int xusb_filter_bypath(const struct xusb_device *xusb_device, void *data)
{
	const char	*p;
	const char	*path = data;

	DBG("%s\n", path);
	assert(path != NULL);
	p = path_tail(path);
	if (strcmp(xusb_device->devpath_tail, p) != 0) {
		DBG("%s: device path missmatch (!= '%s')\n",
			xusb_device->devpath_tail, p);
		return 0;
	}
	return 1;
}

struct xusb_iface *xusb_open_one(const struct xusb_spec *specs, int numspecs,
		int interface_num,
		xusb_filter_t filterfunc, void *data)
{
	struct xlist_node	*xusb_list;
	struct xlist_node	*curr;
	int			num;
	struct xusb_device	*xusb_device = NULL;
	struct xusb_iface	*iface = NULL;
	int ret;

	xusb_list = xusb_find_byproduct(specs, numspecs, filterfunc, data);
	num = xlist_length(xusb_list);
	DBG("total %d devices\n", num);
	switch (num) {
	case 0:
		ERR("No matching device.\n");
		break;
	case 1:
		curr = xlist_shift(xusb_list);
		xusb_device = curr->data;
		xlist_destroy(curr, NULL);
		xlist_destroy(xusb_list, NULL);
		ret = xusb_claim(xusb_device, interface_num, &iface);
		if (ret < 0) {
			ERR("%s: Failed claiming interface %d (ret = %d)\n",
				xusb_device->devpath_tail,
				interface_num,
				ret);
			xusb_destroy(xusb_device);
			return NULL;
		}
		break;
	default:
		ERR("Too many devices (%d). Aborting.\n", num);
		break;
	}
	return iface;
}

int xusb_interface_num(const struct xusb_iface *iface)
{
	return iface->interface_num;
}

uint16_t xusb_vendor_id(const struct xusb_device *xusb_device)
{
	return xusb_device->idVendor;
}

uint16_t xusb_product_id(const struct xusb_device *xusb_device)
{
	return  xusb_device->idProduct;
}

size_t xusb_packet_size(const struct xusb_device *xusb_device)
{
	return xusb_device->packet_size;
}

const char *xusb_serial(const struct xusb_device *xusb_device)
{
	return xusb_device->iSerialNumber;
}

const char *xusb_devpath(const struct xusb_device *xusb_device)
{
	return xusb_device->devpath_tail;
}

uint16_t xusb_bus_num(const struct xusb_device *xusb_device)
{
	return xusb_device->bus_num;
}

uint16_t xusb_device_num(const struct xusb_device *xusb_device)
{
	return xusb_device->device_num;
}

const char *xusb_interface_name(const struct xusb_iface *iface)
{
	return iface->iInterface;
}

const char *xusb_manufacturer(const struct xusb_device *xusb_device)
{
	return xusb_device->iManufacturer;
}

const char *xusb_product(const struct xusb_device *xusb_device)
{
	return xusb_device->iProduct;
}

const struct xusb_spec *xusb_spec(const struct xusb_device *xusb_device)
{
	return xusb_device->spec;
}

int xusb_flushread(struct xusb_iface *iface)
{
	char tmpbuf[BUFSIZ];
	int ret;

	XUSB_DBG(iface, "starting...\n");
	memset(tmpbuf, 0, BUFSIZ);
	ret = xusb_recv(iface, tmpbuf, BUFSIZ, 1);
	if (ret < 0 && ret != -ETIMEDOUT) {
		XUSB_ERR(iface, "ret=%d\n", ret);
		return ret;
	} else if (ret > 0) {
		XUSB_DBG(iface, "Got %d bytes:\n", ret);
		dump_packet(LOG_DEBUG, DBG_MASK, __func__, tmpbuf, ret);
	}
	return 0;
}

static int use_clear_halt = 0;

static int xtalk_one_option(const char *option_string)
{
	if (strcmp(option_string, "use-clear-halt") == 0) {
		use_clear_halt = 1;
		return 0;
	}
	if (strcmp(option_string, "no-use-clear-halt") == 0) {
		use_clear_halt = 0;
		return 0;
	}
	ERR("Unknown XTALK_OPTIONS content: '%s'\n", option_string);
	return -EINVAL;
}

int xtalk_option_use_clear_halt(void)
{
	return use_clear_halt;
}

static void chomp(char *buf)
{
	char *p;
	int len;

	if (!buf)
		return;
	len = strlen(buf);
	for (p = buf + len - 1; p >= buf && isspace(*p); p--)
		*p = '\0';
}

static const char *OPTION_VAR = "XTALK_OPTIONS";

/* Caller should free the returned string if it is not NULL */
static char *read_options(const char *fname)
{
	FILE *fp;
	char buf[BUFSIZ];
	char *p;
	char *ret_buf;

	fp = fopen(fname, "r");
	if (!fp) {
		DBG("Failed opening '$fname': %s\n", strerror(errno));
		return NULL;
	}
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		chomp(buf);
		if (buf[0] == '\0' || buf[0] == '#')
			continue;
		if (strncmp(buf, OPTION_VAR, strlen(OPTION_VAR)) != 0)
			continue;
		/* fprintf(stderr, "INPUT> '%s'\n", p); */
		p = buf + strlen(OPTION_VAR);
		while (*p && (isspace(*p) || *p == '='))
			p++;
		ret_buf = malloc(sizeof(buf));
		strcpy(ret_buf, p); /* Cannot overflow */
		return ret_buf;
	}
	fclose(fp);
	return NULL;
}

int xtalk_parse_options(void)
{
	char *xtalk_options;
	char *saveptr;
	char *token;
	int ret;
	int free_options = 0;

	xtalk_options = getenv("XTALK_OPTIONS");
	if (!xtalk_options) {
		xtalk_options = read_options(XTALK_OPTIONS_FILE);
		if (!xtalk_options)
			return 0;
		free_options = 1;
	}
	token = strtok_r(xtalk_options, " \t", &saveptr);
	while (token) {
		ret = xtalk_one_option(token);
		if (ret < 0)
			return ret;
		token = strtok_r(NULL, " \t", &saveptr);
	}
	if (free_options)
		free(xtalk_options);
	return 0;
}
