#ifndef	XTALK_PROTO_RAW_H
#define	XTALK_PROTO_RAW_H
/*
 * Written by Oron Peled <oron@actcom.co.il>
 * Copyright (C) 2009, Xorcom
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

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * XTALKSYNC - Base synchronous protocol for our USB devices
 *             It is meant to provide a common base for layered
 *             protocols (dialects)
 */

#include <stdint.h>
#include <stdlib.h>
#include <xtalk/api_defs.h>
#include <xtalk/proto.h>
#include <xtalk/firmware_defs.h>

#ifdef	__GNUC__
#define	PACKED	__attribute__((packed))
#else
#error "We do not know how your compiler packs structures"
#endif

/*
 * Base XTALK device. A pointer to this struct
 * should be included in the struct representing
 * the dialect.
 */
struct xtalk_raw;
struct xusb;

XTALK_API struct xtalk_raw *xtalk_raw_new(struct xtalk_base *xtalk_base);
XTALK_API void xtalk_raw_delete(struct xtalk_raw *xraw);
XTALK_API int xtalk_raw_set_protocol(struct xtalk_raw *xtalk_base,
		const struct xtalk_protocol *xproto);
XTALK_API int xtalk_raw_cmd_recv(struct xtalk_raw *xraw,
	struct xtalk_command **reply_ref);
XTALK_API int xtalk_raw_cmd_send(struct xtalk_raw *xraw, const char *buf, int len,
       uint16_t *tx_seq);

/*
 * These are low-level interfaces that receive/send arbitrary buffers
 * Be carefull, as that allow to send illegal Xtalk packets
 */
XTALK_API int xtalk_raw_buffer_recv(struct xtalk_raw *xraw, char *buf, int len);
XTALK_API int xtalk_raw_buffer_send(struct xtalk_raw *xraw, const char *buf, int len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* XTALK_PROTO_RAW_H */
