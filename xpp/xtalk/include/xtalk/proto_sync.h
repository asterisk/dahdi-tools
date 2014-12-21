#ifndef	XTALK_PROTO_SYNC_H
#define	XTALK_PROTO_SYNC_H
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

#include <xtalk/api_defs.h>
#include <xtalk/proto.h>

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
struct xtalk_sync;
struct xusb;

/* high-level */
XTALK_API struct xtalk_sync *xtalk_sync_new(struct xtalk_base *xtalk_base);
XTALK_API void xtalk_sync_delete(struct xtalk_sync *xtalk_sync);
XTALK_API int xtalk_sync_set_protocol(struct xtalk_sync *xtalk_base,
		const struct xtalk_protocol *xproto);
XTALK_API int xtalk_proto_query(struct xtalk_sync *dev);

/* low-level */
XTALK_API int process_command(
	struct xtalk_sync *dev,
	struct xtalk_command *cmd,
	struct xtalk_command **reply_ref,
	uint16_t *sequence_number);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* XTALK_PROTO_SYNC_H */
