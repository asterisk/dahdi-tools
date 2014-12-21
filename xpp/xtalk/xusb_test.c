#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include <autoconfig.h>

static char	*progname;

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

#define	XUSB_IFACE_DESC(p, i, d) \
		{ SPEC_HEAD(0xe4e4, (p), (d)), (i) }

static struct xusb_iface_description {
	struct xusb_spec	spec;
	int interface_num;
} test_specs[] = {
	/* 115x */
	XUSB_IFACE_DESC(0x1150, 1, "old-astribank-NOFW"),
	XUSB_IFACE_DESC(0x1151, 1, "old-astribank-USB"),
	XUSB_IFACE_DESC(0x1152, 1, "old-astribank-FPGA"),

	/* 116x */
	XUSB_IFACE_DESC(0x1160, 1, "astribank2-NOFW"),
	XUSB_IFACE_DESC(0x1161, 1, "astribank2-USB"),
	XUSB_IFACE_DESC(0x1162, 1, "astribank2-FPGA"),
	XUSB_IFACE_DESC(0x1163, 1, "astribank2-TWINSTAR"),
	XUSB_IFACE_DESC(0x1191, 1, "xpanel"),
	XUSB_IFACE_DESC(0x1183, 1, "multi-ps"),
	XUSB_IFACE_DESC(0x11a3, 0, "auth-dongle"),
	XUSB_IFACE_DESC(0xbb01, 0, "iwc"),
	{},
};

static int run_spec(int i, xusb_filter_t filter, char *devpath)
{
	struct xusb_iface_description *desc = &test_specs[i];
	struct xusb_spec	*s = &desc->spec;
	struct xlist_node	*xlist;
	struct xlist_node	*curr;
	int interface_num = desc->interface_num;
	int num_devs;

	if (!s->name)
		return 0;
	xlist = xusb_find_byproduct(s, 1, filter, devpath);
	num_devs = (xlist) ? xlist_length(xlist) : 0;
	INFO("total %d devices of type %s (interface=%d)\n",
		num_devs, s->name, interface_num);
	for (curr = xlist_shift(xlist); curr; curr = xlist_shift(xlist)) {
		struct xusb_device	*xusb_device;
		struct xusb_iface	*iface;
                int ret;

		xusb_device = curr->data;
                ret = xusb_claim(xusb_device, interface_num, &iface);
                if (ret < 0) {
                        ERR("%s: xusb_claim() failed (ret = %d)\n",
                                xusb_devpath(xusb_device), ret);
                        continue;
                }
		xusb_showinfo(xusb_device);
                xusb_release(iface);
	}
	xlist_destroy(xlist, xusb_destructor);
	return 1;
}

int main(int argc, char *argv[])
{
	char			*devpath = NULL;
	const char		options[] = "vd:D:EFp";
	xusb_filter_t		filter = NULL;
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
	while (run_spec(i, filter, devpath))
		i++;
	return 0;
}
