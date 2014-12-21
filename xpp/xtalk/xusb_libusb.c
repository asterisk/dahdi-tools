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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <usb.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include <autoconfig.h>
#include "xusb_common.h"

#define	DBG_MASK	0x01
#define	TIMEOUT	500
#define	MAX_RETRIES	10

#define	EP_IS_IN(ep)	(((ep) & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_IN)
#define	EP_IS_OUT(ep)	(((ep) & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_OUT)

struct libusb_implementation {
	struct usb_device	*dev;
	usb_dev_handle		*handle;
	struct usb_config_descriptor	*config_desc;
};

static void xusb_init();

/*
 * USB handling
 */

static int get_usb_string(struct xusb_device *xusb_device, uint8_t item, char *buf)
{
	char	tmp[BUFSIZ];
	int	ret;

	if (!xusb_device->impl->handle) {
		ERR("%s: device closed\n", xusb_device->devpath_tail);
		return -ENXIO;
	}
	if (!item)
		return 0;
	ret = usb_get_string_simple(xusb_device->impl->handle, item, tmp, BUFSIZ);
	if (ret <= 0)
		return ret;
	return snprintf(buf, BUFSIZ, "%s", tmp);
}

static const struct usb_interface_descriptor *get_iface_descriptor(
	const struct xusb_device *xusb_device, int i)
{
	const struct usb_config_descriptor	*config_desc;
	const struct usb_interface		*interface;
	const struct usb_interface_descriptor	*iface_desc;
	int					num_altsetting;

	assert(xusb_device);
	assert(xusb_device->impl);
	config_desc = xusb_device->impl->config_desc;
	assert(config_desc);
	assert(config_desc->bNumInterfaces < XUSB_MAX_INTERFACES);
	if (i >= XUSB_MAX_INTERFACES)
		return NULL;
	interface = &config_desc->interface[i];
	assert(interface != NULL);
	iface_desc = interface->altsetting;
	num_altsetting = interface->num_altsetting;
	assert(num_altsetting != 0);
	assert(iface_desc != NULL);
	return iface_desc;
}

#define	GET_USB_STRING(xusb_device, from, item) \
		get_usb_string((xusb_device), (from)->item, (xusb_device)->item)

static int xusb_fill_strings(struct xusb_device *xusb_device, int interface_num)
{
	const struct usb_device_descriptor *dev_desc;
	const struct usb_interface_descriptor *iface_desc;
	struct xusb_iface *iface = xusb_device->interfaces[interface_num];
	int ret;

	dev_desc = &xusb_device->impl->dev->descriptor;
	assert(dev_desc);
	ret = GET_USB_STRING(xusb_device, dev_desc, iManufacturer);
	if (ret < 0) {
		XUSB_ERR(iface, "Failed reading iManufacturer string: %s\n",
			usb_strerror());
		return 0;
	}
	ret = GET_USB_STRING(xusb_device, dev_desc, iProduct);
	if (ret < 0) {
		XUSB_ERR(iface, "Failed reading iProduct string: %s\n",
			usb_strerror());
		return 0;
	}
	ret = GET_USB_STRING(xusb_device, dev_desc, iSerialNumber);
	if (ret < 0) {
		XUSB_ERR(iface, "Failed reading iSerialNumber string: %s\n",
			usb_strerror());
		return 0;
	}
	iface_desc = get_iface_descriptor(xusb_device, interface_num);
	if (!iface_desc) {
		XUSB_ERR(iface, "Could not get interface descriptor of device\n");
		return 0;
	}
	ret = GET_USB_STRING(xusb_device, iface_desc, iInterface);
	if (ret < 0) {
		XUSB_ERR(iface, "Failed reading iInterface string: %s\n",
			usb_strerror());
		return 0;
	}
	return 1;
}

static int xusb_open(struct xusb_device *xusb_device)
{
	int ret = 0;

	assert(xusb_device);
	DBG("%s\n", xusb_device->devpath_tail);
	if (xusb_device->impl->handle) {
		ERR("%s: already open\n", xusb_device->devpath_tail);
		ret = -EBUSY;
		goto out;
	}
	xusb_device->impl->handle = usb_open(xusb_device->impl->dev);
	if (!xusb_device->impl->handle) {
		ERR("%s: Failed to open usb device: %s\n",
			xusb_device->devpath_tail,
			usb_strerror());
		xusb_device->impl->handle = NULL;
		goto out;
	}
	return 1;
out:
	return ret;
}

void xusb_release(struct xusb_iface *iface)
{
	if (iface && iface->is_claimed) {
		usb_dev_handle *handle;
		int ret;

		assert(iface->xusb_device);
		handle = iface->xusb_device->impl->handle;
		XUSB_DBG(iface, "Releasing interface\n");
		if (!handle) {
			XUSB_ERR(iface, "device closed\n");
			iface->is_claimed = 0;
			return;
		}
		ret = usb_release_interface(handle, iface->interface_num);
		if (ret < 0)
			XUSB_ERR(iface, "Releasing interface: %s\n",
					usb_strerror());
		iface->is_claimed = 0;
	}
}

static int xusb_clear_halt(struct xusb_iface *xusb_iface)
{
	struct xusb_device *xusb_device;
	int ret = 0;
	int ep;

	xusb_device = xusb_iface->xusb_device;
	/*
	 * WE DO NOT CALL HALT for problematic devices:
	 * - It cause problem with our usb-dongle (cypress CY7C63803, interrupt driven)
	 */
	if (xusb_device->idVendor == 0xe4e4 && xusb_device->idProduct == 0x11a3) {
		XUSB_DBG(xusb_iface, "Skipping clear_halt()\n");
		goto out;
	}
	if (!xtalk_option_use_clear_halt()) {
		XUSB_DBG(xusb_iface, "Don't use clear_halt()\n");
		goto out;
	}
	ep = EP_OUT(xusb_iface);
	ret = usb_clear_halt(xusb_device->impl->handle, ep);
	if (ret < 0) {
		XUSB_ERR(xusb_iface, "Clearing output endpoint 0x%02X: %s\n",
			ep, usb_strerror());
		goto out;
	}
	ep = EP_IN(xusb_iface);
	ret = usb_clear_halt(xusb_device->impl->handle, ep);
	if (ret < 0) {
		XUSB_ERR(xusb_iface, "Clearing input endpoint 0x%02X: %s\n",
			ep, usb_strerror());
		goto out;
	}
out:
	return ret;
}

int xusb_claim(struct xusb_device *xusb_device, unsigned int interface_num,
	struct xusb_iface **xusb_iface)
{
	struct xusb_iface *iface = NULL;
	enum xusb_transfer_type	iface_tt = XUSB_TT_ILLEGAL;
	int ret = 0;

	*xusb_iface = NULL;
	assert(xusb_device);
	if (!xusb_device->impl->handle) {
		XUSB_ERR(iface, "device closed\n");
		ret = -ENXIO;
		goto failed;
	}
	if (interface_num >= XUSB_MAX_INTERFACES) {
		ERR("%s: interface number %d is too big\n",
			xusb_device->devpath_tail, interface_num);
		ret = -EINVAL;
		goto failed;
	}
	iface = xusb_device->interfaces[interface_num];
	if (!iface) {
		ERR("%s: No interface number %d\n",
			xusb_device->devpath_tail, interface_num);
		ret = -EINVAL;
		goto failed;
	}
	if (iface->is_claimed) {
		XUSB_ERR(iface, "Already claimed\n");
		ret = -EBUSY;
		goto failed;
	}
	ret = usb_claim_interface(xusb_device->impl->handle, iface->interface_num);
	if (ret < 0) {
		XUSB_ERR(iface, "usb_claim_interface: %s\n", usb_strerror());
		goto failed;
	}
	iface->is_claimed = 1;
	iface_tt = xusb_transfer_type(iface);
	if (iface_tt == XUSB_TT_ILLEGAL) {
		ret = -ENOTSUP;
		goto failed;
	}
	iface->transfer_type = iface_tt;
	ret = xusb_clear_halt(iface);
	if (ret < 0)
		goto failed;
	ret = xusb_flushread(iface);
	if (ret < 0) {
		XUSB_ERR(iface, "xusb_flushread failed: %s\n", usb_strerror());
		goto failed;
	}
	xusb_fill_strings(xusb_device, interface_num);
	XUSB_DBG(iface, "ID=%04X:%04X Manufacturer=[%s] Product=[%s] "
		"SerialNumber=[%s] Interface=[%s] TT=%s\n",
		xusb_device->idVendor,
		xusb_device->idProduct,
		xusb_device->iManufacturer,
		xusb_device->iProduct,
		xusb_device->iSerialNumber,
		iface->iInterface,
		xusb_tt_name(iface->transfer_type));
	*xusb_iface = iface;
	return 0;
failed:
	xusb_release(iface);
	return ret;
}

void xusb_destroy(struct xusb_device *xusb_device)
{
	if (xusb_device) {
		struct xusb_iface **piface;
		struct libusb_implementation *impl;

		for (piface = xusb_device->interfaces; *piface; piface++) {
			xusb_destroy_interface(*piface);
			*piface = NULL;
		}
		impl = xusb_device->impl;
		if (impl) {
			if (impl->handle) {
				xusb_close(xusb_device);
				impl->handle = NULL;
			}
		}
		DBG("%s: MEM: FREE device\n", xusb_device->devpath_tail);
		memset(xusb_device, 0, sizeof(*xusb_device));
		free(xusb_device);
		xusb_device = NULL;
	}
}

static int init_interfaces(struct xusb_device *xusb_device)
{
	const struct usb_config_descriptor *config_desc;
	const struct usb_interface_descriptor *iface_desc;
	struct xusb_iface *iface;
	int max_packet_size = 0;
	int packet_size;
	int if_idx;

	assert(xusb_device);
	assert(xusb_device->impl);
	config_desc = xusb_device->impl->config_desc;
	assert(config_desc);
	assert(config_desc->bNumInterfaces < XUSB_MAX_INTERFACES);
	for (if_idx = 0; if_idx < config_desc->bNumInterfaces; if_idx++) {
		int ep_idx;

		iface_desc = get_iface_descriptor(xusb_device, if_idx);
		if (iface_desc->bInterfaceNumber != if_idx) {
			ERR("%s: interface %d is number %d\n",
				xusb_device->devpath_tail,
				if_idx, iface_desc->bInterfaceNumber);
			return -EINVAL;
		}
		if (iface_desc->bNumEndpoints != 2) {
			ERR("%s: interface %d has %d endpoints\n",
				xusb_device->devpath_tail,
				if_idx, iface_desc->bNumEndpoints);
			return -EINVAL;
		}
		iface = calloc(sizeof(*iface), 1);
		if (!iface) {
			ERR("%s: interface %d -- out of memory\n",
				xusb_device->devpath_tail, if_idx);
			return -ENOMEM;
		}
		DBG("MEM: ALLOC interface: %p\n", iface);
		xusb_device->interfaces[if_idx] = iface;
		iface->xusb_device = xusb_device;
		iface->interface_num = iface_desc->bInterfaceNumber;

		/* Search Endpoints */
		iface->ep_in = 0;
		iface->ep_out = 0;
		for (ep_idx = 0; ep_idx < iface_desc->bNumEndpoints; ep_idx++) {
			int ep_num;

			ep_num = iface_desc->endpoint[ep_idx].bEndpointAddress;
			packet_size = iface_desc->endpoint[ep_idx].wMaxPacketSize;
			if (!max_packet_size || packet_size < max_packet_size)
				max_packet_size = packet_size;
			if (EP_IS_OUT(ep_num))
				iface->ep_out = ep_num;
			if (EP_IS_IN(ep_num))
				iface->ep_in = ep_num;
		}
		/* Validation */
		if (!iface->ep_out) {
			ERR("%s[%d]: Missing output endpoint\n",
				xusb_device->devpath_tail, if_idx);
			return -EINVAL;
		}
		if (!iface->ep_in) {
			ERR("%s[%d]: Missing input endpoint\n",
				xusb_device->devpath_tail, if_idx);
			return -EINVAL;
		}

		iface->is_claimed = 0;
		XUSB_DBG(iface, "ep_out=0x%X ep_in=0x%X packet_size=%d\n",
			iface->ep_out, iface->ep_in, max_packet_size);
	}
	if (xusb_device->packet_size < max_packet_size)
		xusb_device->packet_size = max_packet_size;
	xusb_device->is_usb2 = (xusb_device->packet_size == 512);
	return 0;
}

static struct xusb_device *xusb_new(struct usb_device *dev,
	const struct xusb_spec *spec,
	xusb_filter_t filterfunc,
	void *data)
{
	struct usb_device_descriptor	*dev_desc;
	int				ret;
	struct xusb_device		*xusb_device = NULL;

	xusb_device = calloc(sizeof(*xusb_device) + sizeof(struct libusb_implementation), 1);
	if (!xusb_device) {
		ERR("Out of memory");
		goto fail;
	}
	DBG("MEM: ALLOC device: %p\n", xusb_device);
	/* Fill xusb_device */
	xusb_device->impl = (void *)xusb_device + sizeof(*xusb_device);
	xusb_device->impl->dev = dev;
	xusb_device->spec = spec;
	/*
	 * Get information from the usb_device
	 */
	dev_desc = &dev->descriptor;
	if (!dev_desc) {
		ERR("usb device without a device descriptor\n");
		goto fail;
	}
	xusb_device->idVendor = dev_desc->idVendor;
	xusb_device->idProduct = dev_desc->idProduct;
	sscanf(dev->bus->dirname, "%d", &xusb_device->bus_num);
	sscanf(dev->filename, "%d", &xusb_device->device_num);
	snprintf(xusb_device->devpath_tail, PATH_MAX, "%03d/%03d",
		xusb_device->bus_num, xusb_device->device_num);
	if (!match_device(xusb_device, spec)) {
		DBG("[%04X:%04X] did not match\n",
			xusb_device->idVendor, xusb_device->idProduct);
		goto fail;
	}
	xusb_device->impl->config_desc = dev->config;
	if (!xusb_device->impl->config_desc) {
		ERR("usb device without a configuration descriptor\n");
		goto fail;
	}
	ret = init_interfaces(xusb_device);
	if (ret < 0) {
		ERR("%s: init_interfaces() failed (ret = %d)\n",
			xusb_device->devpath_tail, ret);
		goto fail;
	}
	if (!xusb_open(xusb_device)) {
		ERR("%s: Failed opening device: %04X:%04X\n",
			xusb_device->devpath_tail,
			xusb_device->idVendor,
			xusb_device->idProduct);
		goto fail;
	}
	DBG("%s: %04X:%04X\n",
		xusb_device->devpath_tail,
		xusb_device->idVendor,
		xusb_device->idProduct);
	return xusb_device;
fail:
	xusb_destroy(xusb_device);
	return NULL;
}

struct xusb_iface *xusb_find_iface(const char *devpath,
	int iface_num,
	int ep_out,
	int ep_in,
	struct xusb_spec *dummy_spec)
{
	return NULL;
}

struct xusb_device *xusb_find_bypath(const char *path)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	char devpath_tail[PATH_MAX];
	struct xusb_spec *spec;

	DBG("path='%s'\n", path);
	spec = calloc(sizeof(*spec), 1);
	if (!spec) {
		ERR("Failed allocating spec\n");
		goto failed;
	}
	xusb_init();
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			struct usb_device_descriptor *dev_desc;
			struct xusb_device *xusb_device = NULL;

			dev_desc = &dev->descriptor;
			assert(dev_desc);
			DBG("usb:%s/%s: ID=%04X:%04X\n",
				dev->bus->dirname,
				dev->filename,
				dev_desc->idVendor,
				dev_desc->idProduct);
			snprintf(devpath_tail, PATH_MAX, "%3s/%3s",
				dev->bus->dirname, dev->filename);
			if (!match_devpath(path, devpath_tail))
				continue;
			DBG("Found: usb:%s/%s: ID=%04X:%04X\n",
				dev->bus->dirname,
				dev->filename,
				dev_desc->idVendor,
				dev_desc->idProduct);
			xusb_init_spec(spec, "<BYPATH>",
				dev_desc->idVendor, dev_desc->idProduct);
			xusb_device = xusb_new(dev, spec, NULL, NULL);
			if (!xusb_device) {
				ERR("Failed creating xusb for %s\n",
					devpath_tail);
				xusb_init_spec(spec, "<EMPTY>", 0, 0);
				continue;
			}
			return xusb_device;
		}
	}
failed:
	if (spec)
		free(spec);
	return NULL;
}

struct xlist_node *xusb_find_byproduct(const struct xusb_spec *specs,
		int numspecs, xusb_filter_t filterfunc, void *data)
{
	struct xlist_node	*xlist;
	struct usb_bus		*bus;
	struct usb_device	*dev;

	DBG("specs(%d)\n", numspecs);
	xlist = xlist_new(NULL);
	if (!xlist) {
		ERR("Failed allocation new xlist");
		goto failed;
	}
	xusb_init();
	for (bus = usb_get_busses(); bus; bus = bus->next) {
		for (dev = bus->devices; dev; dev = dev->next) {
			struct usb_device_descriptor	*dev_desc;
			struct xlist_node		*item;
			int				i;

			dev_desc = &dev->descriptor;
			assert(dev_desc);
			DBG("usb:%s/%s: ID=%04X:%04X\n",
				dev->bus->dirname,
				dev->filename,
				dev_desc->idVendor,
				dev_desc->idProduct);
			for (i = 0; i < numspecs; i++) {
				struct xusb_device	*xusb_device;
				const struct xusb_spec	*sp = &specs[i];

				xusb_device = xusb_new(dev, sp, filterfunc, data);
				if (!xusb_device)
					continue;
				if (filterfunc && !filterfunc(xusb_device, data)) {
					DBG("%s: %04X:%04X filtered out\n",
						xusb_device->devpath_tail,
						dev_desc->idVendor,
						dev_desc->idProduct);
					xusb_destroy(xusb_device);
					continue;
				}
				item = xlist_new(xusb_device);
				xlist_append_item(xlist, item);
				break;
			}
		}
	}
	xusb_list_dump(xlist);
	return xlist;
failed:
	if (xlist)
		xlist_destroy(xlist, NULL);
	return NULL;
}

void xusb_showinfo(const struct xusb_device *xusb_device)
{
	struct usb_device_descriptor	*dev_desc;
	struct usb_device		*dev;
	const struct xusb_iface **piface;

	assert(xusb_device);
	dev = xusb_device->impl->dev;
	assert(dev);
	dev_desc = &dev->descriptor;
	assert(dev_desc);
	if (verbose <= LOG_INFO) {
		INFO("%s: [%04X:%04X] [%s / %s / %s]\n",
			xusb_device->devpath_tail,
			dev_desc->idVendor,
			dev_desc->idProduct,
			xusb_device->iManufacturer,
			xusb_device->iProduct,
			xusb_device->iSerialNumber);
	} else {
		printf("USB    Bus/Device:    [%s/%s] (%s)\n",
			dev->bus->dirname,
			dev->filename,
			(xusb_device->impl->handle) ? "open" : "closed");
		printf("USB    Spec name:     [%s]\n", xusb_device->spec->name);
		printf("USB    iManufacturer: [%s]\n", xusb_device->iManufacturer);
		printf("USB    iProduct:      [%s]\n", xusb_device->iProduct);
		printf("USB    iSerialNumber: [%s]\n", xusb_device->iSerialNumber);
		piface = (const struct xusb_iface **)xusb_device->interfaces;
		for (; *piface; piface++) {
			printf("USB    Interface[%d]:  ep_out=0x%02X ep_in=0x%02X claimed=%d [%s]\n",
				(*piface)->interface_num,
				(*piface)->ep_out,
				(*piface)->ep_in,
				(*piface)->is_claimed,
				(*piface)->iInterface);
		}
	}
}

int xusb_close(struct xusb_device *xusb_device)
{
	if (xusb_device && xusb_device->impl && xusb_device->impl->handle) {
		assert(xusb_device->spec);
		assert(xusb_device->spec->name);
		DBG("%s: Closing device \"%s\"\n",
			xusb_device->devpath_tail,
			xusb_device->spec->name);
		usb_close(xusb_device->impl->handle);
		xusb_device->impl->handle = NULL;
	}
	return 0;
}

enum xusb_transfer_type xusb_transfer_type(const struct xusb_iface *iface)
{
	const struct xusb_device *xusb_device;
	const struct usb_interface_descriptor *iface_desc;
	const struct usb_endpoint_descriptor *ep;
	enum xusb_transfer_type ret = XUSB_TT_ILLEGAL;

	assert(iface);
	xusb_device = iface->xusb_device;
	assert(xusb_device);
	iface_desc = get_iface_descriptor(xusb_device, iface->interface_num);
	assert(iface_desc);
	ep = iface_desc->endpoint;
	assert(ep);
	switch (ep->bmAttributes) {
        case USB_ENDPOINT_TYPE_CONTROL:
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
		break;
        case USB_ENDPOINT_TYPE_BULK:
		ret = XUSB_TT_BULK;
		break;
        case USB_ENDPOINT_TYPE_INTERRUPT:
		ret = XUSB_TT_INTERRUPT;
		break;
	}
	return ret;
}

int xusb_send(struct xusb_iface *iface, const char *buf, int len, int timeout)
{
	struct xusb_device *xusb_device = iface->xusb_device;
	int ep_out = EP_OUT(iface);
	int retries = 0;
	int ret;

	dump_packet(LOG_DEBUG, DBG_MASK, __func__, buf, len);
	if (!xusb_device->impl->handle) {
		XUSB_ERR(iface, "device closed\n");
		return -ENXIO;
	}
	if (!EP_IS_OUT(ep_out)) {
		XUSB_ERR(iface, "%s called with an input endpoint 0x%x\n",
			__func__, ep_out);
		return -EINVAL;
	}
retry_write:
	switch (iface->transfer_type) {
	case XUSB_TT_BULK:
		ret = usb_bulk_write(xusb_device->impl->handle, ep_out, (char *)buf, len, timeout);
		break;
	case XUSB_TT_INTERRUPT:
		ret = usb_interrupt_write(xusb_device->impl->handle, ep_out, (char *)buf, len, timeout);
		break;
	default:
		ret = -EINVAL;
		break;
	}
	if (ret < 0) {
		/*
		 * If the device was gone, it may be the
		 * result of renumeration. Ignore it.
		 */
		if (ret != -ENODEV) {
			XUSB_ERR(iface, "write to endpoint 0x%x failed: (%d) %s\n",
				ep_out, ret, usb_strerror());
			dump_packet(LOG_ERR, DBG_MASK, "xusb_send[ERR]",
				buf, len);
			/*exit(2);*/
		} else {
			XUSB_DBG(iface, "write to endpoint 0x%x got ENODEV\n",
				ep_out);
			xusb_close(xusb_device);
		}
		return ret;
	}
	if (!ret) {

		XUSB_ERR(iface, "write to endpoint 0x%x short write[%d]: (%d)\n",
			ep_out, retries, ret);
		if (retries++ > MAX_RETRIES)
			return -EFAULT;
		usleep(100);
		goto retry_write;
	}
	if (ret != len) {
		XUSB_ERR(iface, "write to endpoint 0x%x short write: (%d) %s\n",
			ep_out, ret, usb_strerror());
		dump_packet(LOG_ERR, DBG_MASK, "xusb_send[ERR]", buf, len);
		return -EFAULT;
	}
	return ret;
}

int xusb_recv(struct xusb_iface *iface, char *buf, size_t len, int timeout)
{
	struct xusb_device *xusb_device = iface->xusb_device;
	int ep_in = EP_IN(iface);
	int retries = 0;
	int ret;

	if (!xusb_device->impl->handle) {
		XUSB_ERR(iface, "device closed\n");
		return -ENXIO;
	}
	if (!EP_IS_IN(ep_in)) {
		XUSB_ERR(iface, "called with an output endpoint 0x%x\n", ep_in);
		return -EINVAL;
	}
retry_read:
	switch (iface->transfer_type) {
	case XUSB_TT_BULK:
		ret = usb_bulk_read(xusb_device->impl->handle, ep_in, buf, len, timeout);
		break;
	case XUSB_TT_INTERRUPT:
		ret = usb_interrupt_read(xusb_device->impl->handle, ep_in, buf, len, timeout);
		break;
	default:
		ret = -EAFNOSUPPORT;
		break;
	}
	if (ret < 0) {
		XUSB_DBG(iface, "read from endpoint 0x%x failed: (%d) %s\n",
			ep_in, ret, usb_strerror());
		memset(buf, 0, len);
		return ret;
	}
	if (!ret) {
		XUSB_ERR(iface, "read to endpoint 0x%x short read[%d]: (%d)\n",
			ep_in, retries, ret);
		if (retries++ > MAX_RETRIES)
			return -EFAULT;
		usleep(100);
		goto retry_read;
	}
	dump_packet(LOG_DEBUG, DBG_MASK, __func__, buf, ret);
	return ret;
}

/*
 * Serialize calls to usb_find_busses()/usb_find_devices()
 */

static const key_t	SEM_KEY = 0x1a2b3c4d;
static int semid = -1;	/* Failure */

static void xusb_lock_usb()
{
	struct sembuf	sembuf;

	while (semid < 0) {
		/* Maybe it was already created? */
		semid = semget(SEM_KEY, 1, 0);
		if (semid < 0) {
			/* No, let's create ourselves */
			semid = semget(SEM_KEY, 1, IPC_CREAT | IPC_EXCL | 0644);
			if (semid < 0) {
				/* Someone else won the race to create it */
				if (errno != ENOENT)
					ERR("%s: semget() failed: %s\n",
						__func__, strerror(errno));
				/* Retry */
				continue;
			}
			/* Initialize */
			if (semctl(semid, 0, SETVAL, 1) < 0)
				ERR("%s: SETVAL() failed: %s\n",
					__func__, strerror(errno));
		}
	}
	DBG("%d: LOCKING\n", getpid());
	sembuf.sem_num = 0;
	sembuf.sem_op = -1;
	sembuf.sem_flg = SEM_UNDO;
	if (semop(semid, &sembuf, 1) < 0)
		ERR("%s: semop() failed: %s\n", __func__, strerror(errno));
	DBG("%d: LOCKED\n", getpid());
}

static void xusb_unlock_usb()
{
	struct sembuf	sembuf;

	DBG("%d: UNLOCKING\n", getpid());
	sembuf.sem_num = 0;
	sembuf.sem_op = 1;
	sembuf.sem_flg = SEM_UNDO;
	if (semop(semid, &sembuf, 1) < 0)
		ERR("%s: semop() failed: %s\n", __func__, strerror(errno));
	DBG("%d: UNLOCKED\n", getpid());
}

static int		initizalized;

void __attribute__((constructor)) xusb_init(void)
{
	xtalk_parse_options();
	if (!getenv("XUSB_NOLOCK"))
		xusb_lock_usb();
	if (!initizalized) {
		usb_init();
		initizalized = 1;
	}
	usb_find_busses();
	usb_find_devices();
	if (!getenv("XUSB_NOLOCK"))
		xusb_unlock_usb();
}
