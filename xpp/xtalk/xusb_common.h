#ifndef	XUSB_COMMON_H
#define	XUSB_COMMON_H
/*
 * Written by Oron Peled <oron@actcom.co.il>
 * Copyright (C) 2012, Xorcom
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

#include <limits.h>
#include <xtalk/xusb.h>

/*
 * XTALK_OPTIONS:
 *     - "use-clear-halt"
 *     - "no-use-clear-halt"
 */
int xtalk_parse_options(void);
int xtalk_option_use_clear_halt(void);

enum xusb_transfer_type {
	XUSB_TT_ILLEGAL = 0,
	XUSB_TT_BULK,
	XUSB_TT_INTERRUPT,
};

struct xusb_iface {
	struct xusb_device	*xusb_device;
	int		interface_num;
	int		ep_out;
	int		ep_in;
	enum xusb_transfer_type transfer_type;
	int		is_claimed;
	char		iInterface[BUFSIZ];
};
struct libusb_implementation;

#define	XUSB_MAX_INTERFACES	32

struct xusb_device {
	struct libusb_implementation *impl;
	const struct xusb_spec	*spec;
	int			idVendor;
	int			idProduct;
	char			iManufacturer[BUFSIZ];
	char			iProduct[BUFSIZ];
	char			iSerialNumber[BUFSIZ];
	char			iInterface[BUFSIZ];
	char			devpath_tail[PATH_MAX + 1];
	int			bus_num;
	int			device_num;
	int			is_usb2;
	size_t			packet_size;
	struct xusb_iface	*interfaces[XUSB_MAX_INTERFACES];
};

#define	EP_OUT(iface)	((iface)->ep_out)
#define	EP_IN(iface)	((iface)->ep_in)

int match_devpath(const char *path, const char *tail);
int match_device(const struct xusb_device *xusb_device,
		const struct xusb_spec *spec);
void xusb_list_dump(struct xlist_node *xusb_list);
void xusb_destroy_interface(struct xusb_iface *iface);
int xusb_close(struct xusb_device *xusb_device);

enum xusb_transfer_type xusb_transfer_type(const struct xusb_iface *iface);
const char *xusb_tt_name(enum xusb_transfer_type tt);


int xusb_printf(const struct xusb_iface *iface, int level, int debug_mask,
	const char *prefix, const char *fmt, ...);

int xusb_printf_details(const struct xusb_iface *iface, int level, int debug_mask,
	const char *file, int line, const char *severity, const char *func,
	const char *fmt, ...);

#define XUSB_PRINT(iface, level, fmt, arg...) \
		xusb_printf(iface, LOG_ ## level, 0, #level ": ", fmt, ## arg)

#define XUSB_PRINT_DETAILS(iface, level, debug_mask, fmt, arg...) \
		xusb_printf_details(iface, LOG_ ## level, debug_mask, \
			__FILE__, __LINE__, #level, __func__, fmt, ## arg)

#define XUSB_INFO(iface, fmt, arg...) XUSB_PRINT(iface, INFO, fmt, ## arg)
#define XUSB_ERR(iface, fmt, arg...) XUSB_PRINT_DETAILS(iface, ERR, 0, fmt, ## arg)
#define XUSB_DBG(iface, fmt, arg...) XUSB_PRINT_DETAILS(iface, DEBUG, DBG_MASK, fmt, ## arg)

#endif	/* XUSB_COMMON_H */
