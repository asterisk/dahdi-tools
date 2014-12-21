#ifndef	XUSB_H
#define	XUSB_H
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
#include <stdint.h>
#include <xtalk/api_defs.h>
#include <xtalk/xlist.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Xorcom usb handling
 */

#define	PACKET_SIZE	512

/*
 * Specify the wanted device
 */
struct xusb_spec {
	char	*name;	/* For debug/output purpose */
	/* What we will actually use */
	uint16_t	vendor_id;
	uint16_t	product_id;
};

#define	SPEC_HEAD(vendor_id_, product_id_, name_) \
	{ \
		.name = (name_), \
		.vendor_id = (vendor_id_), \
		.product_id = (product_id_), \
	}

XTALK_API void xusb_init_spec(struct xusb_spec *xusb_spec,
	char *name, uint16_t vendor_id, uint16_t product_id);

struct xusb_device;
struct xusb_iface;

/*
 * Prototypes
 */
typedef int (*xusb_filter_t)(const struct xusb_device *xusb_device, void *data);
XTALK_API struct xlist_node *xusb_find_byproduct(const struct xusb_spec *specs,
		int numspecs, xusb_filter_t filterfunc, void *data);
XTALK_API struct xusb_device *xusb_find_bypath(const char *path);
XTALK_API struct xusb_iface *xusb_open_one(const struct xusb_spec *specs, int numspecs,
		int interface_num,
		xusb_filter_t filterfunc, void *data);

/*
 * A convenience filter
 */
XTALK_API int xusb_filter_bypath(const struct xusb_device *xusb_device, void *data);

/* Device management */
XTALK_API const struct xusb_spec *xusb_spec(const struct xusb_device *xusb_device);
XTALK_API void xusb_destroy(struct xusb_device *xusb_device);
XTALK_API size_t xusb_packet_size(const struct xusb_device *xusb_device);
XTALK_API void xusb_showinfo(const struct xusb_device *xusb_device);
XTALK_API const char *xusb_serial(const struct xusb_device *xusb_device);
XTALK_API const char *xusb_manufacturer(const struct xusb_device *xusb_device);
XTALK_API const char *xusb_product(const struct xusb_device *xusb_device);
XTALK_API uint16_t xusb_bus_num(const struct xusb_device *xusb_device);
XTALK_API uint16_t xusb_device_num(const struct xusb_device *xusb_device);
XTALK_API uint16_t xusb_vendor_id(const struct xusb_device *xusb_device);
XTALK_API uint16_t xusb_product_id(const struct xusb_device *xusb_device);
XTALK_API const char *xusb_devpath(const struct xusb_device *xusb_device);
XTALK_API const struct xusb_spec *xusb_device_spec(const struct xusb_device *xusb_device);
XTALK_API struct xusb_iface *xusb_find_iface(const char *devpath,
	int iface_num,
	int ep_out,
	int ep_in,
	struct xusb_spec *dummy_spec);
XTALK_API int xusb_claim(struct xusb_device *xusb_device, unsigned int interface_num,
	struct xusb_iface **iface);
XTALK_API void xusb_release(struct xusb_iface *iface);
XTALK_API int xusb_is_claimed(struct xusb_iface *iface);
XTALK_API struct xusb_iface *xusb_interface_of(const struct xusb_device *dev, int num);
XTALK_API struct xusb_device *xusb_deviceof(struct xusb_iface *iface);
XTALK_API const char *xusb_interface_name(const struct xusb_iface *iface);
XTALK_API int xusb_interface_num(const struct xusb_iface *iface);
XTALK_API int xusb_send(struct xusb_iface *iface, const char *buf, int len, int timeout);
XTALK_API int xusb_recv(struct xusb_iface *iface, char *buf, size_t len, int timeout);
XTALK_API int xusb_flushread(struct xusb_iface *iface);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* XUSB_H */
