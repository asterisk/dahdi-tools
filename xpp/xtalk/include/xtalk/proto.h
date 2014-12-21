#ifndef	XTALK_PROTO_H
#define	XTALK_PROTO_H
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
 * XTALK - Base protocol for our USB devices
 *         It is meant to provide a common base for layered
 *         protocols (dialects)
 */

#include <stdint.h>
#include <stdlib.h>
#include <xtalk/api_defs.h>
#include <xtalk/firmware_defs.h>

#ifdef	__GNUC__
#define	PACKED	__attribute__((packed))
#else
#error "We do not know how your compiler packs structures"
#endif

struct xtalk_base;
struct xtalk_command_desc;
struct xtalk_command;

/*
 * Callbacks should return negative errno's
 * in case of errors.
 * They are called from process_command() and their
 * return values are propagated back.
 */
typedef int (*xtalk_cmd_callback_t)(
	const struct xtalk_base *xtalk_base,
	const struct xtalk_command_desc *cmd_desc,
	struct xtalk_command *cmd);

/* Describe a single xtalk command */
struct xtalk_command_desc {
	uint8_t			op;
	const char		*name;
	uint16_t		len;	/* Minimal length */
};

/* Define a complete protocol */
struct xtalk_protocol {
	const char			*name;
	uint8_t				proto_version;
	struct xtalk_command_desc	commands[MAX_OPS];
	const char			*ack_statuses[MAX_STATUS];
};

/*
 * The common header of every xtalk command
 * in every xtalk dialect.
 */
struct xtalk_header {
	uint16_t	len;
	uint16_t	seq;
	uint8_t		op;	/* MSB: 0 - to device, 1 - from device */
} PACKED;

struct xtalk_command {
	/* Common part */
	struct xtalk_header	header;
	/* Each dialect has its own data members */
	union private_data {
		uint8_t	raw_data[0];
	} PACKED alt;
} PACKED;

/*
 * Macros to unify access to protocol packets and fields:
 *   p		- signify the dialect prefix (XTALK for base protocol)
 *   o		- signify command op (e.g: ACK)
 *   cmd	- A pointer to struct xtalk_command
 *   field	- field name (e.g: raw_data)
 */
#define XTALK_STRUCT(p, o)	p ## _struct_ ## o
#define XTALK_PDATA(o)		xtalk_privdata_ ## o
#define	XTALK_CMD_PTR(cmd, p)	((union XTALK_PDATA(p)*)&((cmd)->alt))
#define	CMD_FIELD(cmd, p, o, field) \
		(XTALK_CMD_PTR(cmd, p)->XTALK_STRUCT(p, o).field)
#define CMD_DEF(p, o, ...)	struct XTALK_STRUCT(p, o) {	\
					__VA_ARGS__		\
				} PACKED XTALK_STRUCT(p, o)
#define	MEMBER(p, o)	struct XTALK_STRUCT(p, o) XTALK_STRUCT(p, o)
#define	XTALK_OP(p, o)		(p ## _ ## o)

/* Wrappers for transport (xusb) functions */
struct xtalk_ops {
	int	(*send_func)(void *transport_priv, const char *data, size_t len,
			int timeout);
	int	(*recv_func)(void *transport_priv, char *data, size_t maxlen,
			int timeout);
	int	(*close_func)(void *transport_priv);
};

/*
 * Base XTALK device. A pointer to this struct
 * should be included in the struct representing
 * the dialect.
 */
struct xtalk_base;
struct xusb_iface;

/* high-level */
XTALK_API struct xtalk_base *xtalk_base_new_on_xusb(struct xusb_iface *xusb_iface);
XTALK_API struct xtalk_base *xtalk_base_new(const struct xtalk_ops *ops,
	size_t packet_size, void *priv);
XTALK_API void xtalk_base_delete(struct xtalk_base *xtalk_base);
XTALK_API struct xusb_iface *xusb_iface_of_xtalk_base(const struct xtalk_base *xtalk_base);
XTALK_API const char *xtalk_protocol_name(const struct xtalk_base *dev);
XTALK_API int xtalk_cmd_callback(struct xtalk_base *xtalk_base, int op,
		xtalk_cmd_callback_t callback,
		xtalk_cmd_callback_t *old_callback);
XTALK_API void xtalk_dump_command(struct xtalk_command *cmd);
XTALK_API int xtalk_set_timeout(struct xtalk_base *dev, int new_timeout);

/* low-level */
XTALK_API const char *ack_status_msg(const struct xtalk_protocol *xproto,
		uint8_t status);
XTALK_API struct xtalk_command *new_command(
	const struct xtalk_base *xtalk_base,
	uint8_t op, uint16_t extra_data);
XTALK_API void free_command(struct xtalk_command *cmd);

/*
 * Convenience macros to define entries in a protocol command table:
 *   p		- signify the dialect prefix (XTALK for base protocol)
 *   o		- signify command op (e.g: ACK)
 */
#define	CMD_RECV(p, o)	\
	[p ## _ ## o | XTALK_REPLY_MASK] = {		\
		.op = (p ## _ ## o) | XTALK_REPLY_MASK,	\
		.name = (#o "_reply"),			\
		.len =					\
			sizeof(struct xtalk_header) +	\
			sizeof(struct XTALK_STRUCT(p, o)),	\
	}

#define	CMD_SEND(p, o)	\
	[p ## _ ## o] = {		\
		.op = (p ## _ ## o),	\
		.name = (#o),		\
		.len =					\
			sizeof(struct xtalk_header) +	\
			sizeof(struct XTALK_STRUCT(p, o)),	\
	}

/*
 * Convenience macro to define statuses:
 *   x		- status code (e.g: OK)
 *   m		- status message (const char *)
 */
#define	ACK_STAT(x, m)	[STAT_ ## x] = (m)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* XTALK_PROTO_H */
