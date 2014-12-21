#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include <xtalk/proto_raw.h>
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
	struct xusb_device *xusb_device = data;
	xusb_destroy(xusb_device);
}

static const struct test_struct {
	struct xusb_spec	spec;
	int			interface_num;
} known_devices[] = {
	{ SPEC_HEAD(0xe4e4, 0x1162, "astribank2-FPGA"), 0 },
	{{}, 0 },
};

int proto_ack_cb(
	const struct xtalk_base *xtalk_base,
	const struct xtalk_command_desc *cmd_desc,
	struct xtalk_command *cmd)
{
	const char *name = (cmd_desc->name) ? cmd_desc->name : "RAW";

	printf("%s: op=0x%X (%s): len=%d\n", __func__,
		cmd_desc->op, name, cmd->header.len);
	return 0;
}

/*
 * Not const, because we override proto_version for testing
 */
static struct xtalk_protocol   xtalk_raw_test_base = {
	.name   = "XTALK_TEST",
	.proto_version = 0,	/* Modified in test_device() */
	.commands = {
	},
	.ack_statuses = {
	}
};

static int test_device(struct xusb_iface *xusb_iface, int timeout)
{
	struct xtalk_base *xtalk_base = NULL;
	struct xtalk_raw *xtalk_raw = NULL;
	struct xtalk_command *reply;
	uint16_t tx_seq;
	int ret;

	xtalk_base = xtalk_base_new_on_xusb(xusb_iface);
	if (!xtalk_base) {
		ERR("Failed creating the xtalk device abstraction\n");
		ret = -ENOMEM;
		goto err;
	}
	xtalk_raw = xtalk_raw_new(xtalk_base);
	if (!xtalk_raw) {
		ERR("Failed creating the xtalk sync device abstraction\n");
		ret = -ENOMEM;
		goto err;
	}
	ret = xtalk_set_timeout(xtalk_base, timeout);
	INFO("Original timeout=%d, now set to %d\n", ret, timeout);
	/* override constness for testing */
	ret = xtalk_raw_set_protocol(xtalk_raw, &xtalk_raw_test_base);
	if (ret < 0) {
		ERR("%s Protocol registration failed: %d\n",
			xtalk_raw_test_base.name, ret);
		ret = -EPROTO;
		goto err;
	}
	ret = xtalk_cmd_callback(xtalk_base, XTALK_OP(XTALK, ACK), proto_ack_cb, NULL);
	if (ret < 0) {
		ERR("%s Callback registration failed: %d\n",
			xtalk_raw_test_base.name, ret);
		ret = -EPROTO;
		goto err;
	}
	INFO("Device and Protocol are ready\n");
	ret = xtalk_raw_cmd_send(xtalk_raw, "abcdef", 6, &tx_seq);
	if (ret < 0) {
		ERR("Failed sending raw command: %d\n", ret);
		ret = -EPROTO;
		goto err;
	}
	do {
		ret = xtalk_raw_cmd_recv(xtalk_raw, &reply);
		if (ret < 0) {
			if (ret == -ETIMEDOUT) {
				printf("timeout\n");
				continue;
			}
			ERR("Read error (ret=%d)\n", ret);
			goto err;
		}
		assert(reply);
		printf("Got %d (len=%d)\n", reply->header.op, reply->header.len);
	} while (1);
err:
	if (xtalk_raw)
		xtalk_raw_delete(xtalk_raw);
	if (xtalk_base)
		xtalk_base_delete(xtalk_base);
	return ret;
}

static int run_spec(int i, xusb_filter_t filter, char *devpath, int timeout)
{
	const struct xusb_spec	*s = &known_devices[i].spec;
	int			interface_num = known_devices[i].interface_num;
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
			ret = test_device(xusb_iface, timeout);
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
