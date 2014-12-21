#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include <xtalk/proto_sync.h>
#include <autoconfig.h>

static char	*progname;

#define	DBG_MASK	0x10

static void usage()
{
	fprintf(stderr, "Usage: %s [options...] -D <device> [hexnum ....]\n",
		progname);
	fprintf(stderr, "\tOptions:\n");
	fprintf(stderr, "\t\t[-v]               # Increase verbosity\n");
	fprintf(stderr,
		"\t\t[-d mask]          # Debug mask (0xFF for everything)\n");
	fprintf(stderr, "\tDevice:\n");
	fprintf(stderr, "\t\t/proc/bus/usb/<bus>/<dev>\n");
	fprintf(stderr, "\t\t/dev/bus/usb/<bus>/<dev>\n");
	exit(1);
}

static void xusb_destructor(void *data)
{
	struct xusb_device	*xusb_device = data;
	xusb_destroy(xusb_device);
}

#define	KNOWN_DEV(p, i, v, d) \
		{ SPEC_HEAD(0xe4e4, (p), (d)), (i), (v) }

static const struct test_struct {
	struct xusb_spec	spec;
	int			interface_num;
	uint8_t			proto_version;
} known_devices[] = {
	/*        PROD    I  V     NAME */
	KNOWN_DEV(0x1161, 1, 0x14, "astribank2-USB"),
	KNOWN_DEV(0x1162, 1, 0x14, "astribank2-FPGA"),
	KNOWN_DEV(0x1191, 1, 0x10, "xpanel"),
	KNOWN_DEV(0x1183, 1, 0x11, "multi-ps"),
	KNOWN_DEV(0x11a3, 0, 0x01, "auth-dongle"),
	KNOWN_DEV(0xbb01, 1, 0x10, "iwc"),
	KNOWN_DEV(0,      0, 0,    NULL),
};

int proto_get_reply_cb(
	const struct xtalk_base *xtalk_base,
	const struct xtalk_command_desc *cmd_desc,
	struct xtalk_command *cmd)
{
	INFO("CALLBACK: op=0x%X (%s): len=%d\n",
		cmd_desc->op, cmd_desc->name, cmd->header.len);
	return 0;
}

/*
 * Not const, because we override proto_version for testing
 */
static struct xtalk_protocol   xtalk_test_base = {
	.name   = "XTALK_TEST",
	.proto_version = 0,	/* Modified in test_device() */
	.commands = {
	},
	.ack_statuses = {
	}
};

static int test_device(struct xusb_iface *xusb_iface, uint8_t wanted_version, int timeout)
{
	struct xtalk_base *xtalk_base;
	struct xtalk_sync *xtalk_sync;
	int proto_version;
	int ret;

	xtalk_base = xtalk_base_new_on_xusb(xusb_iface);
	if (!xtalk_base) {
		ERR("Failed creating the xtalk device abstraction\n");
		return -ENOMEM;
	}
	xtalk_sync = xtalk_sync_new(xtalk_base);
	if (!xtalk_sync) {
		ERR("Failed creating the xtalk sync device abstraction\n");
		return -ENOMEM;
	}
	ret = xtalk_set_timeout(xtalk_base, timeout);
	INFO("Original timeout=%d, now set to %d\n", ret, timeout);
	/* override constness for testing */
	xtalk_test_base.proto_version = wanted_version;
	ret = xtalk_sync_set_protocol(xtalk_sync, &xtalk_test_base);
	if (ret < 0) {
		ERR("%s Protocol registration failed: %d\n",
			xtalk_test_base.name, ret);
		return -EPROTO;
	}
	ret = xtalk_cmd_callback(xtalk_base, XTALK_OP(XTALK, PROTO_GET_REPLY), proto_get_reply_cb, NULL);
	if (ret < 0) {
		ERR("%s Callback registration failed: %d\n",
			xtalk_test_base.name, ret);
		return -EPROTO;
	}
	proto_version = xtalk_proto_query(xtalk_sync);
	if (proto_version < 0) {
		ERR("Protocol query error: %s\n", strerror(-proto_version));
		return proto_version;
	}
	if (proto_version != xtalk_test_base.proto_version) {
		ERR("Bad protocol version: 0x%02x\n", proto_version);
		return -EPROTO;
	}
	INFO("Device and Protocol are ready (proto_version=0x%X)\n",
		proto_version);
	xtalk_sync_delete(xtalk_sync);
	xtalk_base_delete(xtalk_base);
	return 0;
}

static int run_spec(int i, xusb_filter_t filter, char *devpath, int timeout)
{
	const struct xusb_spec	*s = &known_devices[i].spec;
	int			interface_num = known_devices[i].interface_num;
	uint8_t			proto_version = known_devices[i].proto_version;
	struct xlist_node	*xlist;
	struct xlist_node	*curr;
	struct xusb_device	*xusb_device;
	int			success = 1;

	if (!s->name)
		return 0;
	xlist = xusb_find_byproduct(s, 1, filter, devpath);
	if (!xlist_length(xlist))
		return 1;
	INFO("total %zd devices of type %s\n", xlist_length(xlist), s->name);
	for (curr = xlist_shift(xlist); curr; curr = xlist_shift(xlist)) {
		struct xusb_iface *xusb_iface;
		int ret;

		xusb_device = curr->data;
		xusb_showinfo(xusb_device);
		INFO("Testing interface %d\n", interface_num);
		ret = xusb_claim(xusb_device, interface_num, &xusb_iface);
		if (ret == 0) {
			ret = test_device(xusb_iface, proto_version, timeout);
			if (ret < 0)
				success = 0;
		}
		xusb_destroy(xusb_device);
	}
	xlist_destroy(xlist, xusb_destructor);
	return success;
}

int main(int argc, char *argv[])
{
	char			*devpath = NULL;
	const char		options[] = "vd:D:EFpt:";
	xusb_filter_t		filter = NULL;
	int			timeout = 500;	/* millies */
	int			i;

	progname = argv[0];
	while (1) {
		int	c;

		c = getopt(argc, argv, options);
		if (c == -1)
			break;

		switch (c) {
		case 'D':
			devpath = optarg;
			filter = xusb_filter_bypath;
			break;
		case 'v':
			verbose++;
			break;
		case 't':
			timeout = strtoul(optarg, NULL, 0);
			break;
		case 'd':
			debug_mask = strtoul(optarg, NULL, 0);
			break;
		case 'h':
		default:
			ERR("Unknown option '%c'\n", c);
			usage();
		}
	}
	i = 0;
	while (run_spec(i, filter, devpath, timeout))
		i++;
	return 0;
}
