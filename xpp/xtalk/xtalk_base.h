#ifndef	XTALK_BASE_H
#define	XTALK_BASE_H
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

#include <stdint.h>
#include <stdlib.h>
#include <xtalk/proto.h>
#include <xtalk/firmware_defs.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Base XTALK device. A pointer to this struct
 * should be included in the struct representing
 * the dialect.
 */
struct xtalk_base {
	void			*transport_priv;	/* e.g: struct xusb */
	struct xtalk_ops	ops;
	struct xtalk_protocol	xproto;
	xtalk_cmd_callback_t	callbacks[MAX_OPS];
	uint8_t			xtalk_proto_version;
	uint8_t                 status;
	size_t			packet_size;
	uint16_t                tx_sequenceno;
	int			default_timeout;	/* in millies */
};

int xtalk_set_protocol(struct xtalk_base *xtalk_base,
		const struct xtalk_protocol *xproto_base,
		const struct xtalk_protocol *xproto);
const struct xtalk_command_desc *get_command_desc(
		const struct xtalk_protocol *xproto, uint8_t op);
int send_command(struct xtalk_base *xtalk_base,
		struct xtalk_command *cmd, uint16_t *tx_seq);
int recv_command(struct xtalk_base *xtalk_base,
		struct xtalk_command **reply_ref);

/*
 * These are low-level interfaces that receive/send arbitrary buffers
 * Be carefull, as that allow to send illegal Xtalk packets
 */
int send_buffer(struct xtalk_base *xtalk_base, const char *buf, int len);
int recv_buffer(struct xtalk_base *xtalk_base, char *buf, int len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* XTALK_BASE_H */
