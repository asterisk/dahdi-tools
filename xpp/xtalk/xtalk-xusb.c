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

/*
 * Convenience wrappers for xtalk_base over xusb
 */
#include <assert.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include <autoconfig.h>
#include "xtalk_base.h"

static inline int close_func(void *priv)
{
	struct xusb_iface *iface = (struct xusb_iface *)priv;
	xusb_release(iface);
        return 0;
}

static inline int send_func(void *priv, const char *data, size_t len, int timeout)
{
	return xusb_send((struct xusb_iface *)priv, data, len, timeout);
}

static inline int recv_func(void *priv, char *data, size_t maxlen, int timeout)
{
	return xusb_recv((struct xusb_iface *)priv, data, maxlen, timeout);
}


static struct xtalk_ops	xtalk_ops = {
	.send_func	= send_func,
	.recv_func	= recv_func,
	.close_func	= close_func,
};

struct xtalk_base *xtalk_base_new_on_xusb(struct xusb_iface *xusb_iface)
{
	struct xtalk_base	*xtalk_base;
	int packet_size;

	assert(xusb_iface);
	packet_size = xusb_packet_size(xusb_deviceof(xusb_iface));

	xtalk_base = xtalk_base_new(&xtalk_ops, packet_size, xusb_iface);
	if (!xtalk_base) {
		ERR("Failed creating the xtalk device abstraction\n");
		return NULL;
	}
	return xtalk_base;
}
