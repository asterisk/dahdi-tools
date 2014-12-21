/*
 * Written by Oron Peled <oron@actcom.co.il>
 * Copyright (C) 2008-2011, Xorcom
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include <xtalk/proto_sync.h>
#include <autoconfig.h>

#define	DBG_MASK	0x80

static char	*progname;
static int	timeout		= 500;	/* msec */
static int	iface_num	= 1;

static void usage()
{
	fprintf(stderr, "Usage: %s [options...] -D <busnum>/<devnum> [hexnum ....]\n",
		progname);
	fprintf(stderr, "\tOptions:\n");
	fprintf(stderr,
		"\t\t[-I<iface_num>]    # Interface number (default %d)\n",
		iface_num);
	fprintf(stderr,
		"\t\t[-t<timeout>]      # Timeout (default %d)\n",
		timeout);
	fprintf(stderr, "\t\t[-Q]               # Query protocol version\n");
	fprintf(stderr, "\t\t[-v]               # Increase verbosity\n");
	fprintf(stderr,
		"\t\t[-d mask]          # Debug mask (0xFF for everything)\n");
	fprintf(stderr, "\t\t[-h]               # This help\n");
	exit(1);
}

/***** XTALK Interface *****************************************************/
static const struct xtalk_protocol   xtalk_protocol = {
	.name   = "XTALK-DERIVED",
	.proto_version = 0,
	.commands = {
	},
	.ack_statuses = {
	}
};


/***** XUSB Interface ******************************************************/

static int sendto_device(struct xusb_iface *iface, int nargs, char *args[])
{
	char *reply = NULL;
	char *buf = NULL;
	int ret = 0;
	int i;

	assert(nargs >= 0);
	if (!nargs)
		goto out;
	buf = malloc(nargs);
	if (!buf) {
		ERR("Out of memory for %d command bytes\n", nargs);
		ret = -ENOMEM;
		goto out;
	}
	for (i = 0; i < nargs; i++) {
		int	val = strtoul(args[i], NULL, 16);
		printf("%d> 0x%02X\n", i, val);
		buf[i] = val;
	}
	ret = xusb_send(iface, buf, nargs, timeout);
	if (ret < 0) {
		ERR("xusb_send failed ret=%d\n", ret);
		goto out;
	}
	reply = malloc(PACKET_SIZE);
	if (!reply) {
		ERR("Out of memory\n");
		ret = -ENOMEM;
		goto out;
	}
	ret = xusb_recv(iface, reply, PACKET_SIZE, timeout);
	if (ret < 0) {
		ERR("Receive from usb failed.\n");
		goto out;
	}
	dump_packet(LOG_INFO, 0, "REPLY", reply, ret);
	ret = 0;
out:
	if (reply)
		free(reply);
	if (buf)
		free(buf);
	return ret;
}

static int show_protocol(struct xtalk_sync *xtalk_sync, struct xusb_iface *iface)
{
	int	ret;

	ret = xtalk_sync_set_protocol(xtalk_sync, &xtalk_protocol);
	if (ret < 0) {
		ERR("%s Protocol registration failed: %s\n",
			xtalk_protocol.name, strerror(-ret));
		return ret;
	}
	ret = xtalk_proto_query(xtalk_sync);
	if (ret < 0) {
		ERR("Protocol query error: %s\n", strerror(-ret));
		return ret;
	}
	INFO("usb:%s: Protocol version 0x%X\n", xusb_devpath(xusb_deviceof(iface)), ret);
	return 0;
}

int main(int argc, char *argv[])
{
	char			*devpath = NULL;
	struct xusb_device	*xusb_device;
	struct xtalk_base	*xtalk_base;
	struct xtalk_sync	*xtalk_sync;
	struct xusb_iface	*iface;
	const char		options[] = "vd:D:t:I:i:o:Q";
	int			query = 0;
	int			ret;

	progname = argv[0];
	while (1) {
		int	c;

		c = getopt(argc, argv, options);
		if (c == -1)
			break;

		switch (c) {
		case 'D':
			devpath = optarg;
			break;
		case 'I':
			iface_num = strtoul(optarg, NULL, 0);
			break;
		case 't':
			timeout = strtoul(optarg, NULL, 0);
			break;
		case 'Q':
			query++;
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
	if (!devpath) {
		ERR("Missing device path\n");
		usage();
	}
	xusb_device = xusb_find_bypath(devpath);
	if (!xusb_device) {
		ERR("No XUSB device found\n");
		return 1;
	}
	ret = xusb_claim(xusb_device, iface_num, &iface);
	if (ret < 0) {
		ERR("Claiming interface #%d failed (ret = %d)\n", iface_num, ret);
		return 1;
	}
	xusb_showinfo(xusb_deviceof(iface));
	xtalk_base = xtalk_base_new_on_xusb(iface);
	if (!xtalk_base) {
		ERR("Failed creating the xtalk device abstraction\n");
		return 1;
	}
	xtalk_sync = xtalk_sync_new(xtalk_base);
	if (!xtalk_sync) {
		ERR("Failed creating the xtalk device abstraction\n");
		return 1;
	}
	if (query) {
		ret = show_protocol(xtalk_sync, iface);
		if (ret < 0)
			return 1;
	}
	ret = sendto_device(iface, argc - optind, argv + optind);
	if (ret < 0)
		ERR("Command failed: %d\n", ret);
	xtalk_sync_delete(xtalk_sync);
	xtalk_base_delete(xtalk_base);
	xusb_destroy(xusb_deviceof(iface));
	return 0;
}
