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
#include <xtalk/proto_sync.h>
#include <autoconfig.h>
#include "xtalk_base.h"

#define	DBG_MASK	0x02

/*
 * Base XTALK device. A pointer to this struct
 * should be included in the struct representing
 * the dialect.
 */
struct xtalk_sync {
	struct xtalk_base	*xtalk_base;
};

CMD_DEF(XTALK, ACK,
	uint8_t stat;
	);

CMD_DEF(XTALK, PROTO_GET,
	uint8_t proto_version;
	uint8_t reserved;
	);

CMD_DEF(XTALK, PROTO_GET_REPLY,
	uint8_t proto_version;
	uint8_t reserved;
	);

union XTALK_PDATA(XTALK) {
	MEMBER(XTALK, ACK);
	MEMBER(XTALK, PROTO_GET);
	MEMBER(XTALK, PROTO_GET_REPLY);
} PACKED members;

const struct xtalk_protocol	xtalk_sync_proto = {
	.name	= "XTALK-SYNC",
	.proto_version = 0,
	.commands = {
		CMD_RECV(XTALK, ACK),
		CMD_SEND(XTALK, PROTO_GET),
		CMD_RECV(XTALK, PROTO_GET_REPLY),
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

struct xtalk_sync *xtalk_sync_new(struct xtalk_base *xtalk_base)
{
	struct xtalk_sync *xtalk_sync;
	int ret;

	assert(xtalk_base);
	xtalk_sync = calloc(1, sizeof(*xtalk_sync));
	if (!xtalk_sync) {
		ERR("Allocating XTALK device memory failed\n");
		return NULL;
	}
	xtalk_sync->xtalk_base = xtalk_base;
	ret = xtalk_set_protocol(xtalk_sync->xtalk_base, &xtalk_sync_proto, NULL);
	if (ret < 0) {
		ERR("GLOBAL Protocol registration failed: %d\n", ret);
		goto err;
	}
        DBG("%s: xtalk_sync=%p\n", __func__, xtalk_sync);
	return xtalk_sync;
err:
	xtalk_sync_delete(xtalk_sync);
	return NULL;
}

void xtalk_sync_delete(struct xtalk_sync *xtalk_sync)
{
	if (xtalk_sync) {
		memset(xtalk_sync, 0, sizeof(*xtalk_sync));
		free(xtalk_sync);
	}
}

int xtalk_sync_set_protocol(struct xtalk_sync *xtalk_sync,
		const struct xtalk_protocol *xproto)
{
	return xtalk_set_protocol(xtalk_sync->xtalk_base, &xtalk_sync_proto, xproto);
}

__attribute__((warn_unused_result))
int process_command(
	struct xtalk_sync *xtalk_sync,
	struct xtalk_command *cmd,
	struct xtalk_command **reply_ref,
	uint16_t *tx_seq)
{
	struct xtalk_base *xtalk_base;
	const struct xtalk_protocol	*xproto;
	struct xtalk_command		*reply = NULL;
	const struct xtalk_command_desc	*reply_desc;
	const struct xtalk_command_desc	*expected;
	const struct xtalk_command_desc	*cmd_desc;
	uint8_t				reply_op;
	const char			*protoname;
	int				ret;
	xtalk_cmd_callback_t		callback;

	xtalk_base = xtalk_sync->xtalk_base;
	xproto = &xtalk_base->xproto;
	protoname = (xproto) ? xproto->name : "GLOBAL";
	/* So the caller knows if a reply was received */
	if (reply_ref)
		*reply_ref = NULL;
	reply_op = cmd->header.op | XTALK_REPLY_MASK;
	cmd_desc = get_command_desc(xproto, cmd->header.op);
	expected = get_command_desc(xproto, reply_op);
	ret = send_command(xtalk_base, cmd, tx_seq);
	if (ret < 0) {
		ERR("send_command failed: %d\n", ret);
		goto out;
	}
	if (!reply_ref) {
		DBG("No reply requested\n");
		goto out;
	}
	ret = recv_command(xtalk_base, &reply);
	if (ret <= 0) {
		DBG("recv_command failed (ret = %d)\n", ret);
		goto out;
	}
	*reply_ref = reply;
	if ((reply->header.op & 0x80) != 0x80) {
		ERR("Unexpected reply op=0x%02X, should have MSB set.\n",
			reply->header.op);
		ret = -EPROTO;
		goto out;
	}
	reply_desc = get_command_desc(xproto, reply->header.op);
	if (!reply_desc) {
		ERR("Unknown reply (proto=%s) op=0x%02X\n",
			protoname, reply->header.op);
		dump_packet(LOG_ERR, 0, __func__, (const char *)reply, ret);
		ret = -EPROTO;
		goto out;
	}
	DBG("REPLY OP: 0x%X [%s]\n", reply->header.op, reply_desc->name);
	if (reply->header.op == XTALK_ACK) {
		int	status = CMD_FIELD(reply, XTALK, ACK, stat);

		if (expected) {
			ERR("Expected OP=0x%02X: Got ACK(%d): %s\n",
				reply_op,
				status,
				ack_status_msg(xproto, status));
			ret = -EPROTO;
			goto out;
		} else if (status != STAT_OK) {

			ERR("Got ACK (for OP=0x%X [%s]): %d %s\n",
				cmd->header.op,
				cmd_desc->name,
				status, ack_status_msg(xproto, status));
			ret = -EPROTO;
			goto out;
		}
		/* Good expected ACK ... */
	} else if (reply->header.op != reply_op) {
			ERR("Expected OP=0x%02X: Got OP=0x%02X\n",
				reply_op, reply->header.op);
			ret = -EPROTO;
			goto out;
	}
	if (expected && expected->len > reply->header.len) {
			ERR("Expected len=%d: Got len=%d\n",
				expected->len, reply->header.len);
			ret = -EPROTO;
			goto out;
	}
	if (cmd->header.seq != reply->header.seq) {
			ERR("Expected seq=%d: Got seq=%d\n",
				cmd->header.seq, reply->header.seq);
			ret = -EPROTO;
			goto out;
	}
	/* Find if there is associated callback */
	ret = xtalk_cmd_callback(xtalk_base, reply->header.op, NULL, &callback);
	if (ret < 0) {
		ERR("Failed getting callback for op=0x%X\n", reply->header.op);
		goto out;
	}
	ret = reply->header.len;	/* All good, return the length */
	DBG("got reply op 0x%X (%d bytes)\n", reply->header.op, ret);
	if (callback) {
		/* Override return value with callback return value */
		ret = callback(xtalk_base, reply_desc, reply);
		DBG("%s: callback for 0x%X returned %d\n", __func__,
			reply->header.op, ret);
	}
out:
	free_command(cmd);
	if (!reply_ref && reply)
		free_command(reply);
	return ret;
}

/*
 * Protocol Commands
 */

int xtalk_proto_query(struct xtalk_sync *xtalk_sync)
{
	struct xtalk_base *xtalk_base;
	struct xtalk_command		*cmd;
	struct xtalk_command		*reply;
	uint8_t				proto_version;
	int				ret;
	uint16_t			tx_seq;

	DBG("\n");
	assert(xtalk_sync != NULL);
	xtalk_base = xtalk_sync->xtalk_base;
	proto_version = xtalk_base->xproto.proto_version;
	cmd = new_command(xtalk_base, XTALK_OP(XTALK, PROTO_GET), 0);
	if (!cmd) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	/* Protocol Version */
	CMD_FIELD(cmd, XTALK, PROTO_GET, proto_version) = proto_version;
	CMD_FIELD(cmd, XTALK, PROTO_GET, reserved) = 0;	/* RESERVED */
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if (ret < 0) {
		ERR("process_command failed: %d\n", ret);
		goto out;
	}
	xtalk_base->xtalk_proto_version =
		CMD_FIELD(reply, XTALK, PROTO_GET_REPLY, proto_version);
	if (xtalk_base->xtalk_proto_version != proto_version) {
		DBG("Got %s protocol version: 0x%02x (expected 0x%02x)\n",
			xtalk_base->xproto.name,
			xtalk_base->xtalk_proto_version,
			proto_version);
		ret = xtalk_base->xtalk_proto_version;
		goto out;
	}
	DBG("Protocol version: %02x (tx_seq = %d)\n",
		xtalk_base->xtalk_proto_version, tx_seq);
	ret = xtalk_base->xtalk_proto_version;
out:
	free_command(reply);
	return ret;
}
