#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include <autoconfig.h>

static char	*progname;

static void usage()
{
	fprintf(stderr, "Usage: %s [options...] <busnum>/<devnum> ...\n", progname);
	fprintf(stderr, "\tOptions:\n");
	fprintf(stderr, "\t\t[-v]               # Increase verbosity\n");
	fprintf(stderr,
		"\t\t[-d mask]          # Debug mask (0xFF for everything)\n");
	exit(1);
}

static int check_devpath(const char *devpath)
{
	const struct xusb_spec *spec;
	struct xusb_device *xusb_dev;
	struct xusb_iface *iface;
	int ret;

	printf("Checking %s: ", devpath);
	fflush(stdout);
	xusb_dev = xusb_find_bypath(devpath);
	if (!xusb_dev) {
		ERR("missing %s\n", devpath);
		return 1;
	}
	spec = xusb_device_spec(xusb_dev);
	printf("Found: %04x:%04x\n", spec->vendor_id, spec->product_id);
	ret = xusb_claim(xusb_dev, 1, &iface);
	if (ret < 0) {
		ERR("%s: xusb_claim() failed (ret = %d)\n",
			xusb_devpath(xusb_dev), ret);
		return 1;
	}
	xusb_showinfo(xusb_dev);
	xusb_release(iface);
	return 1;
}

int main(int argc, char *argv[])
{
	const char		options[] = "vd:";
	int			i;

	progname = argv[0];
	while (1) {
		int	c;

		c = getopt(argc, argv, options);
		if (c == -1)
			break;

		switch (c) {
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
	for (i = optind; i < argc; i++)
		check_devpath(argv[i]);
	return 0;
}
