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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>
#include <xtalk/debug.h>
#include <autoconfig.h>
#include "xtalk_base.h"

#define	DBG_MASK	0x02

void free_command(struct xtalk_command *cmd)
{
	if (!cmd)
		return;
	memset(cmd, 0, cmd->header.len);
	free(cmd);
}

const struct xtalk_command_desc *get_command_desc(
		const struct xtalk_protocol *xproto, uint8_t op)
{
	const struct xtalk_command_desc	*desc;

	if (!xproto)
		return NULL;
	desc = &xproto->commands[op];
	if (!desc->name)
		return NULL;
#if 0
	DBG("%s version=%d, op=0x%X (%s)\n",
		xproto->name, xproto->proto_version,
		op, desc->name);
#endif
	return desc;
}

const char *ack_status_msg(const struct xtalk_protocol *xproto,
		uint8_t status)
{
	const char	*ack_status;

	if (!xproto)
		return NULL;
	ack_status = xproto->ack_statuses[status];
	DBG("%s status=0x%X (%s)\n", xproto->name, status, ack_status);
	return ack_status;
}

const char *xtalk_protocol_name(const struct xtalk_base *xtalk_base)
{
	const struct xtalk_protocol	*xproto;

	xproto = &xtalk_base->xproto;
        return (xproto) ? xproto->name : "";
}

int xtalk_set_protocol(struct xtalk_base *xtalk_base,
		const struct xtalk_protocol *xproto_base,
		const struct xtalk_protocol *xproto)
{
	const char	*protoname = (xproto) ? xproto->name : "GLOBAL";
	int		i;

	DBG("%s\n", protoname);
	memset(&xtalk_base->xproto, 0, sizeof(xtalk_base->xproto));
	for (i = 0; i < MAX_OPS; i++) {
		const struct xtalk_command_desc	*desc;

		desc = get_command_desc(xproto, i);
		if (desc) {
			if (!IS_PRIVATE_OP(i)) {
				ERR("Bad op=0x%X "
					"(should be in the range [0x%X-0x%X]\n",
					i, PRIVATE_OP_FIRST, PRIVATE_OP_LAST);
				return -EINVAL;
			}
			xtalk_base->xproto.commands[i] = *desc;
			DBG("private: op=0x%X (%s)\n", i, desc->name);
		} else {
			if (!IS_PRIVATE_OP(i)) {
				const char	*name;

				xtalk_base->xproto.commands[i] =
					xproto_base->commands[i];
				name = xtalk_base->xproto.commands[i].name;
				if (name)
					DBG("global: op=0x%X (%s)\n", i, name);
			}
		}
	}
	for (i = 0; i < MAX_STATUS; i++) {
		const char	*stat_msg;

		stat_msg = (xproto) ? xproto->ack_statuses[i] : NULL;
		if (stat_msg) {
			xtalk_base->xproto.ack_statuses[i] = stat_msg;
			DBG("private: status=0x%X (%s)\n", i, stat_msg);
		} else {
			const char	*global_msg;

			xtalk_base->xproto.ack_statuses[i] =
				xproto_base->ack_statuses[i];
			global_msg = xtalk_base->xproto.ack_statuses[i];
			if (global_msg)
				DBG("global: status=0x%X (%s)\n",
						i, global_msg);
		}
	}
	xtalk_base->xproto.name = protoname;
	xtalk_base->xproto.proto_version = (xproto) ? xproto->proto_version : 0;
	return 0;
}

int xtalk_cmd_callback(struct xtalk_base *xtalk_base, int op,
	xtalk_cmd_callback_t callback,
	xtalk_cmd_callback_t *old_callback)
{
	const struct xtalk_protocol	*xproto;
	const struct xtalk_command_desc	*desc;

	xproto = &xtalk_base->xproto;
	desc = get_command_desc(xproto, op);
	if (!desc)
		DBG("Unknown op=0x%X.\n", op);
	if (old_callback)
		*old_callback = xtalk_base->callbacks[op];
	if (callback) {
		xtalk_base->callbacks[op] = callback;
		DBG("OP=0x%X [%s] -- set callback to %p\n",
			op,
			(desc) ? desc->name : "",
			callback);
	}
	return 0;
}

struct xtalk_command *new_command(
	const struct xtalk_base *xtalk_base,
	uint8_t op, uint16_t extra_data)
{
	const struct xtalk_protocol	*xproto;
	struct xtalk_command		*cmd;
	const struct xtalk_command_desc	*desc;
	uint16_t			len;

	xproto = &xtalk_base->xproto;
	desc = get_command_desc(xproto, op);
	if (!desc) {
		ERR("Unknown op=0x%X.\n", op);
		return NULL;
	}
	DBG("OP=0x%X [%s] (extra_data %d)\n", op, desc->name, extra_data);
	len = desc->len + extra_data;
	cmd = malloc(len);
	if (!cmd) {
		ERR("Out of memory\n");
		return NULL;
	}
	if (extra_data) {
		uint8_t	*ptr = (uint8_t *)cmd;

		DBG("clear extra_data (%d bytes)\n", extra_data);
		memset(ptr + desc->len, 0, extra_data);
	}
	cmd->header.op = op;
	cmd->header.len = len;
	cmd->header.seq = 0;	/* Overwritten in send_usb() */
	return cmd;
}

void xtalk_dump_command(struct xtalk_command *cmd)
{
	uint16_t	len;
	int		i;

	len = cmd->header.len;
	if (len < sizeof(struct xtalk_header)) {
		ERR("Command too short (%d)\n", len);
		return;
	}
	INFO("DUMP: OP=0x%X len=%d seq=%d\n",
		cmd->header.op, cmd->header.len, cmd->header.seq);
	for (i = 0; i < len - sizeof(struct xtalk_header); i++)
		INFO("  %2d. 0x%X\n", i, cmd->alt.raw_data[i]);
}

int xtalk_set_timeout(struct xtalk_base *dev, int new_timeout)
{
	int old_timeout = dev->default_timeout;
	dev->default_timeout = new_timeout;
	return old_timeout;
}

int send_buffer(struct xtalk_base *xtalk_base, const char *buf, int len)
{
	int		ret;
	void		*priv = xtalk_base->transport_priv;
	int		timeout = xtalk_base->default_timeout;

	ret = xtalk_base->ops.send_func(priv, buf, len, timeout);
	if (ret < 0)
		DBG("%s: failed ret=%d\n", __func__, ret);
	return ret;
}

int recv_buffer(struct xtalk_base *xtalk_base, char *buf, int len)
{
	void			*priv = xtalk_base->transport_priv;
	int			timeout = xtalk_base->default_timeout;
	int			ret;

	ret = xtalk_base->ops.recv_func(priv, buf, len, timeout);
	if (ret < 0) {
		DBG("Receive from usb failed (ret=%d)\n", ret);
		goto out;
	} else if (ret == 0) {
		DBG("No reply\n");
		goto out;	/* No reply */
	}
        DBG("Received %d bytes\n", ret);
out:
	return ret;
}

int send_command(struct xtalk_base *xtalk_base,
		struct xtalk_command *cmd, uint16_t *tx_seq)
{
	int		ret;
	int		len;

	len = cmd->header.len;
	cmd->header.seq = xtalk_base->tx_sequenceno;
	ret = send_buffer(xtalk_base, (const char *)cmd, len);
	if (ret < 0)
		DBG("%s: failed ret=%d\n", __func__, ret);
	else if (tx_seq)
		*tx_seq = xtalk_base->tx_sequenceno++;
	return ret;
}

int recv_command(struct xtalk_base *xtalk_base,
		struct xtalk_command **reply_ref)
{
	struct xtalk_command	*reply;
	size_t			psize = xtalk_base->packet_size;
	int			ret;

	reply = malloc(psize);
	if (!reply) {
		ERR("Out of memory\n");
		ret = -ENOMEM;
		goto err;
	}
	reply->header.len = 0;
	ret = recv_buffer(xtalk_base, (char *)reply, psize);
	if (ret < 0) {
		DBG("%s: calling recv_buffer() failed (ret=%d)\n", __func__, ret);
		goto err;
	} else if (ret == 0) {
		goto err;	/* No reply */
	}
	if (ret != reply->header.len) {
		ERR("Wrong length received: got %d bytes, "
			"but length field says %d bytes%s\n",
			ret, reply->header.len,
			(ret == 1) ? ". Old USB firmware?" : "");
		goto err;
	}
	/* dump_packet(LOG_DEBUG, DBG_MASK, __func__, (char *)reply, ret); */
	*reply_ref = reply;
	return ret;
err:
	if (reply) {
		memset(reply, 0, psize);
		free_command(reply);
		*reply_ref = NULL;
	}
	return ret;
}

/*
 * Wrappers
 */

struct xtalk_base *xtalk_base_new(const struct xtalk_ops *ops,
	size_t packet_size, void *priv)
{
	struct xtalk_base *xtalk_base;

	DBG("\n");
	assert(ops != NULL);
	xtalk_base = calloc(1, sizeof(*xtalk_base));
	if (!xtalk_base) {
		ERR("Allocating XTALK device memory failed\n");
		return NULL;
	}
	memcpy((void *)&xtalk_base->ops, (const void *)ops,
		sizeof(xtalk_base->ops));
	xtalk_base->packet_size = packet_size;
	xtalk_base->transport_priv = priv;
	xtalk_base->tx_sequenceno = 1;
	xtalk_base->default_timeout = 2000;	/* millies */
	return xtalk_base;
}

void xtalk_base_delete(struct xtalk_base *xtalk_base)
{
	void	*priv;

	if (!xtalk_base)
		return;
	DBG("\n");
	priv = xtalk_base->transport_priv;
	assert(priv);
	xtalk_base->tx_sequenceno = 0;
	assert(&xtalk_base->ops != NULL);
	assert(&xtalk_base->ops.close_func != NULL);
	xtalk_base->ops.close_func(priv);
	memset(xtalk_base, 0, sizeof(*xtalk_base));
	free(xtalk_base);
}

struct xusb_iface *xusb_iface_of_xtalk_base(const struct xtalk_base *xtalk_base)
{
	return xtalk_base->transport_priv;
}
