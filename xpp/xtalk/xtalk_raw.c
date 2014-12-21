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
#include <xtalk/proto_raw.h>
#include <autoconfig.h>
#include "xtalk_base.h"

#define	DBG_MASK	0x02

/*
 * Base XTALK device. A pointer to this struct
 * should be included in the struct representing
 * the dialect.
 */
struct xtalk_raw {
	struct xtalk_base	*xtalk_base;
};

CMD_DEF(XTALK, ACK,
	uint8_t stat;
	);

union XTALK_PDATA(XTALK) {
	MEMBER(XTALK, ACK);
} PACKED members;

const struct xtalk_protocol	xtalk_raw_proto = {
	.name	= "XTALK-RAW",
	.proto_version = 0,
	.commands = {
		CMD_RECV(XTALK, ACK),
	},
	.ack_statuses = {
		ACK_STAT(OK, "Acknowledges previous command"),
		ACK_STAT(FAIL, "Last command failed"),
		ACK_STAT(RESET_FAIL, "reset failed"),
		ACK_STAT(NODEST, "No destination is selected"),
		ACK_STAT(MISMATCH, "Data mismatch"),
		ACK_STAT(NOACCESS, "No access"),
		ACK_STAT(BAD_CMD, "Bad command"),
		ACK_STAT(TOO_SHORT, "Packet is too short"),
		ACK_STAT(ERROFFS, "Offset error (not used)"),
		ACK_STAT(NO_LEEPROM, "Large EEPROM was not found"),
		ACK_STAT(NO_EEPROM, "No EEPROM was found"),
		ACK_STAT(WRITE_FAIL, "Writing to device failed"),
		ACK_STAT(NOPWR_ERR, "No power on USB connector"),
	}
};

struct xtalk_raw *xtalk_raw_new(struct xtalk_base *xtalk_base)
{
	struct xtalk_raw *xtalk_raw;
	int ret;

	assert(xtalk_base);
	xtalk_raw = calloc(1, sizeof(*xtalk_raw));
	if (!xtalk_raw) {
		ERR("Allocating XTALK device memory failed\n");
		return NULL;
	}
	xtalk_raw->xtalk_base = xtalk_base;
	ret = xtalk_set_protocol(xtalk_raw->xtalk_base, &xtalk_raw_proto, NULL);
	if (ret < 0) {
		ERR("GLOBAL Protocol registration failed: %d\n", ret);
		goto err;
	}
	DBG("%s: xtalk_raw=%p\n", __func__, xtalk_raw);
	return xtalk_raw;
err:
	xtalk_raw_delete(xtalk_raw);
	return NULL;
}

void xtalk_raw_delete(struct xtalk_raw *xtalk_raw)
{
	if (xtalk_raw) {
		memset(xtalk_raw, 0, sizeof(*xtalk_raw));
		free(xtalk_raw);
	}
}

int xtalk_raw_set_protocol(struct xtalk_raw *xtalk_raw,
		const struct xtalk_protocol *xproto)
{
	return xtalk_set_protocol(xtalk_raw->xtalk_base, &xtalk_raw_proto, xproto);
}

int xtalk_raw_cmd_send(struct xtalk_raw *xtalk_raw, const char *buf, int len,
	uint16_t *tx_seq)
{
	struct xtalk_command *cmd;
	char *p;
	int ret;

	p = malloc(len);
	if (!p) {
		ERR("allocation failed (%d bytes)\n", len);
		return -ENOMEM;
	}
	cmd = (struct xtalk_command *)p;
	memcpy(p, buf, len);
	cmd->header.len = len;

	ret = send_command(xtalk_raw->xtalk_base, cmd, tx_seq);
	if (ret < 0) {
		DBG("%s: send_command(%d bytes) failed ret=%d\n",
			__func__, len, ret);
		goto out;
	}
	DBG("%s(%d bytes, tx_seq=%d)\n", __func__, len, *tx_seq);
out:
	free(p);
	return ret;
}

__attribute__((warn_unused_result))
int xtalk_raw_cmd_recv(struct xtalk_raw *xtalk_raw,
	struct xtalk_command **reply_ref)
{
	struct xtalk_base *xtalk_base;
	const struct xtalk_protocol	*xproto;
	struct xtalk_command		*reply = NULL;
	const struct xtalk_command_desc	*reply_desc;
	const char			*protoname;
	int				ret;
	xtalk_cmd_callback_t		callback;

	xtalk_base = xtalk_raw->xtalk_base;
	xproto = &xtalk_base->xproto;
	protoname = (xproto) ? xproto->name : "GLOBAL";
	/* So the caller knows if a reply was received */
	if (reply_ref)
		*reply_ref = NULL;
	ret = recv_command(xtalk_base, &reply);
	if (ret <= 0) {
		DBG("%s: failed (xproto = %s, ret = %d)\n",
			__func__, protoname, ret);
		goto err;
	}
	DBG("REPLY OP: 0x%X\n", reply->header.op);
        if (debug_mask & DBG_MASK)
                xtalk_dump_command(reply);
	/* reply_desc may be NULL (raw reply to application) */
	reply_desc = get_command_desc(xproto, reply->header.op);
	if (reply->header.op == XTALK_ACK) {
		int	status = CMD_FIELD(reply, XTALK, ACK, stat);

		if (status != STAT_OK) {
			ERR("Got NACK(seq=%d): %d %s\n",
				reply->header.seq,
				status,
				ack_status_msg(xproto, status));
		}
		/* Good expected ACK ... */
	}
	DBG("got reply seq=%d op=0x%X (%d bytes)\n",
		reply->header.seq,
		reply->header.op,
		ret);
	/* Find if there is associated callback */
	ret = xtalk_cmd_callback(xtalk_base, reply->header.op, NULL, &callback);
	if (ret < 0) {
		ERR("Failed getting callback for op=0x%X\n", reply->header.op);
		goto err;
	}
	if (callback) {
		/* Override return value with callback return value */
		ret = callback(xtalk_base, reply_desc, reply);
		DBG("%s: callback for 0x%X returned %d\n", __func__,
			reply->header.op, ret);
	}
	if (reply_ref)
		*reply_ref = reply;
	return 0;
err:
	if (reply)
		free_command(reply);
	return ret;
}

int xtalk_raw_buffer_send(struct xtalk_raw *xraw, const char *buf, int len)
{
	dump_packet(LOG_DEBUG, DBG_MASK, __func__, buf, len);
	return send_buffer(xraw->xtalk_base, buf, len);
}

int xtalk_raw_buffer_recv(struct xtalk_raw *xraw, char *buf, int len)
{
	int ret;

	ret = recv_buffer(xraw->xtalk_base, buf, len);
	if (ret >= 0)
		dump_packet(LOG_DEBUG, DBG_MASK, __func__, buf, ret);
	return ret;
}
