/*
 * Written by Oron Peled <oron@actcom.co.il>
 * Copyright (C) 2008, Xorcom
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
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include "mpptalk.h"
#include "astribank.h"

#define	DBG_MASK	0x80
/* if enabled, adds support for resetting pre-MPP USB firmware - if we 
 * failed opening a device and we were asked to reset it, try also the
 * old protocol.
 */
#define SUPPORT_OLD_RESET

static char	*progname;

static void usage()
{
	fprintf(stderr, "Usage: %s [options] -D {/proc/bus/usb|/dev/bus/usb}/<bus>/<dev> [operation...]\n", progname);
	fprintf(stderr, "\tOptions:\n");
	fprintf(stderr, "\t\t[-v]               # Increase verbosity\n");
	fprintf(stderr, "\t\t[-d mask]          # Debug mask (0xFF for everything)\n");
	fprintf(stderr, "\tOperations:\n");
	fprintf(stderr, "\t\t[-n]               # Renumerate device\n");
	fprintf(stderr, "\t\t[-r kind]          # Reset: kind = {half|full}\n");
	fprintf(stderr, "\t\t[-p port]          # TwinStar: USB port number [0, 1]\n");
	fprintf(stderr, "\t\t[-w (0|1)]         # TwinStar: Watchdog off or on guard\n");
	fprintf(stderr, "\t\t[-Q]               # Query device properties\n");
	exit(1);
}

static int reset_kind(const char *arg)
{
	static const struct {
		const char	*name;
		int		type_code;
	} reset_kinds[] = {
		{ "half",	0 },
		{ "full",	1 },
	};
	int	i;

	for(i = 0; i < sizeof(reset_kinds)/sizeof(reset_kinds[0]); i++) {
		if(strcasecmp(reset_kinds[i].name, arg) == 0)
			return reset_kinds[i].type_code;
	}
	ERR("Uknown reset kind '%s'\n", arg);
	return -1;
}

int main(int argc, char *argv[])
{
	char			*devpath = NULL;
	struct astribank *astribank;
	struct mpp_device *mpp;
	const char		options[] = "vd:D:nr:p:w:Q";
	int			opt_renumerate = 0;
	char			*opt_port = NULL;
	char			*opt_watchdog = NULL;
	char			*opt_reset = NULL;
	int			opt_query = 0;
	int			ret;

	progname = argv[0];
	while (1) {
		int	c;

		c = getopt (argc, argv, options);
		if (c == -1)
			break;

		switch (c) {
			case 'D':
				devpath = optarg;
				break;
			case 'n':
				opt_renumerate++;
				break;
			case 'p':
				opt_port = optarg;
				break;
			case 'w':
				opt_watchdog = optarg;
				break;
			case 'r':
				opt_reset = optarg;
				/*
				 * Sanity check so we can reject bad
				 * arguments before device access.
				 */
				if(reset_kind(opt_reset) < 0)
					usage();
				break;
			case 'Q':
				opt_query = 1;
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
	if(!devpath) {
		ERR("Missing device path\n");
		usage();
	}
	DBG("Startup %s\n", devpath);
	astribank = astribank_new(devpath);
	if(!astribank) {
		ERR("Failed initializing Astribank\n");
		return 1;
	}
	mpp = astribank_mpp_open(astribank);
	/*
	 * First process reset options. We want to be able
	 * to reset minimal USB firmwares even if they don't
	 * implement the full MPP protocol (e.g: EEPROM_BURN)
	 */
	if(opt_reset) {
		int	full_reset;

		if((full_reset = reset_kind(opt_reset)) < 0) {
			ERR("Bad reset kind '%s'\n", opt_reset);
			return 1;
		}
		DBG("Reseting (%s)\n", opt_reset);
		if((ret = mpp_reset(mpp, full_reset)) < 0) {
			ERR("%s Reseting astribank failed: %d\n",
				(full_reset) ? "Full" : "Half", ret);
		}
		goto out;
	}
	show_astribank_info(astribank);
	if(opt_query) {
		show_hardware(mpp);
	} else if(opt_renumerate) {
		DBG("Renumerate\n");
		if((ret = mpp_renumerate(mpp)) < 0) {
			ERR("Renumerating astribank failed: %d\n", ret);
		}
	} else if(opt_watchdog) {
		int	watchdogstate = strtoul(opt_watchdog, NULL, 0);

		DBG("TWINSTAR: Setting watchdog %s-guard\n",
			(watchdogstate) ? "on" : "off");
		if((ret = mpp_tws_setwatchdog(mpp, watchdogstate)) < 0) {
			ERR("Failed to set watchdog to %d\n", watchdogstate);
			return 1;
		}
	} else if(opt_port) {
		int	new_portnum = strtoul(opt_port, NULL, 0);
		int	tws_portnum = mpp_tws_portnum(mpp);
		char	*msg = (new_portnum == tws_portnum)
					? " Same same, never mind..."
					: "";

		DBG("TWINSTAR: Setting portnum to %d.%s\n", new_portnum, msg);
		if((ret = mpp_tws_setportnum(mpp, new_portnum)) < 0) {
			ERR("Failed to set USB portnum to %d\n", new_portnum);
			return 1;
		}
	}
out:
	astribank_destroy(astribank);
	return 0;
}
