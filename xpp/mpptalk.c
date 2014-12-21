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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <arpa/inet.h>
#include <xtalk/debug.h>
#include <xtalk/proto.h>
#include "hexfile.h"
#include "mpptalk.h"

#define	DBG_MASK	0x04

enum eeprom_burn_state {
	BURN_STATE_NONE		= 0,
	BURN_STATE_STARTED	= 1,
	BURN_STATE_ENDED	= 2,
	BURN_STATE_FAILED	= 3,
};

const char *eeprom_type2str(int et)
{
	const static char	*msgs[] = {
		[EEPROM_TYPE_NONE]	= "NONE",
		[EEPROM_TYPE_SMALL]	= "SMALL",
		[EEPROM_TYPE_LARGE]	= "LARGE",
		[EEPROM_TYPE_UNUSED]	= "UNUSED",
	};
	if(et > sizeof(msgs)/sizeof(msgs[0]))
		return NULL;
	return msgs[et];
};

const char *dev_dest2str(int dest)
{
	const static char	*msgs[] = {
		[DEST_NONE]	= "NONE",
		[DEST_FPGA]	= "FPGA",
		[DEST_EEPROM]	= "EEPROM",
	};
	if(dest > sizeof(msgs)/sizeof(msgs[0]))
		return NULL;
	return msgs[dest];
};

/*
 * OP Codes:
 * MSB of op signifies a reply from device
 */
#define	MPP_RENUM			0x0B	/* Trigger USB renumeration */
#define	MPP_EEPROM_SET			0x0D

/* AB capabilities	*/
#define	MPP_CAPS_GET			0x0E
#define	MPP_CAPS_GET_REPLY		0x8E
#define	MPP_CAPS_SET			0x0F

#define	MPP_DEV_SEND_START		0x05
#define	MPP_DEV_SEND_SEG		0x07
#define	MPP_DEV_SEND_END		0x09

/* Astribank Status	*/
#define	MPP_STATUS_GET			0x11
#define	MPP_STATUS_GET_REPLY		0x91
#define	MPP_STATUS_GET_REPLY_V13	0x91	/* backward compat */

/* Get extra vendor information	*/
#define	MPP_EXTRAINFO_GET		0x13
#define	MPP_EXTRAINFO_GET_REPLY		0x93
#define	MPP_EXTRAINFO_SET		0x15	/* Set extra vendor information	*/

#define	MPP_EEPROM_BLK_RD		0x27
#define	MPP_EEPROM_BLK_RD_REPLY		0xA7

#define	MPP_SER_SEND			0x37
#define	MPP_SER_RECV			0xB7

#define	MPP_RESET			0x45	/* Reset both FPGA and USB firmwares */
#define	MPP_HALF_RESET			0x47	/* Reset only FPGA firmware */

/* Twinstar */
#define	MPP_TWS_WD_MODE_SET		0x31	/* Set watchdog off/on guard	*/
#define	MPP_TWS_WD_MODE_GET		0x32	/* Current watchdog mode 	*/
#define	MPP_TWS_WD_MODE_GET_REPLY	0xB2	/* Current watchdog mode 	*/
#define	MPP_TWS_PORT_SET		0x34	/* USB-[0/1]			*/
#define	MPP_TWS_PORT_GET		0x35	/* USB-[0/1]			*/
#define	MPP_TWS_PORT_GET_REPLY		0xB5	/* USB-[0/1]			*/
#define	MPP_TWS_PWR_GET			0x36	/* Power: bits -> USB ports	*/
#define	MPP_TWS_PWR_GET_REPLY		0xB6	/* Power: bits -> USB ports	*/

CMD_DEF(MPP, STATUS_GET);

CMD_DEF(MPP, STATUS_GET_REPLY,
	uint8_t i2cs_data;

#define STATUS_FPGA_LOADED(x)   ((x) & 0x01)
	uint8_t status;         /* BIT(0) - FPGA is loaded */
	struct firmware_versions fw_versions;
	);


CMD_DEF(MPP, EEPROM_SET,
	struct eeprom_table	data;
	);

CMD_DEF(MPP, CAPS_GET);

CMD_DEF(MPP, CAPS_GET_REPLY,
	struct eeprom_table	data;
	struct capabilities	capabilities;
	struct capkey		key;
	);

CMD_DEF(MPP, CAPS_SET,
	struct eeprom_table	data;
	struct capabilities	capabilities;
	struct capkey		key;
	);

CMD_DEF(MPP, EXTRAINFO_GET);

CMD_DEF(MPP, EXTRAINFO_GET_REPLY,
	struct extrainfo	info;
	);

CMD_DEF(MPP, EXTRAINFO_SET,
	struct extrainfo	info;
	);

CMD_DEF(MPP, RENUM);

CMD_DEF(MPP, EEPROM_BLK_RD,
	uint16_t	offset;
	uint16_t	len;
	);

CMD_DEF(MPP, EEPROM_BLK_RD_REPLY,
	uint16_t	offset;
	uint8_t		data[0];
	);

CMD_DEF(MPP, DEV_SEND_START,
	uint8_t		dest;
	char		ihex_version[VERSION_LEN];
	);

CMD_DEF(MPP, DEV_SEND_END);

CMD_DEF(MPP, DEV_SEND_SEG,
	uint16_t	offset;
	uint8_t		data[0];
	);

CMD_DEF(MPP, RESET);
CMD_DEF(MPP, HALF_RESET);

CMD_DEF(MPP, SER_SEND,
	uint8_t	data[0];
	);

CMD_DEF(MPP, SER_RECV,
	uint8_t	data[0];
	);

CMD_DEF(MPP, TWS_WD_MODE_SET,
	uint8_t		wd_active;
	);

CMD_DEF(MPP, TWS_WD_MODE_GET);
CMD_DEF(MPP, TWS_WD_MODE_GET_REPLY,
	uint8_t		wd_active;
	);

CMD_DEF(MPP, TWS_PORT_SET,
	uint8_t		portnum;
	);

CMD_DEF(MPP, TWS_PORT_GET);
CMD_DEF(MPP, TWS_PORT_GET_REPLY,
	uint8_t		portnum;
	);

CMD_DEF(MPP, TWS_PWR_GET);
CMD_DEF(MPP, TWS_PWR_GET_REPLY,
	uint8_t		power;
	);


union XTALK_PDATA(MPP) {
		MEMBER(MPP, STATUS_GET);
		MEMBER(MPP, STATUS_GET_REPLY);
		MEMBER(MPP, EEPROM_SET);
		MEMBER(MPP, CAPS_GET);
		MEMBER(MPP, CAPS_GET_REPLY);
		MEMBER(MPP, CAPS_SET);
		MEMBER(MPP, EXTRAINFO_GET);
		MEMBER(MPP, EXTRAINFO_GET_REPLY);
		MEMBER(MPP, EXTRAINFO_SET);
		MEMBER(MPP, RENUM);
		MEMBER(MPP, EEPROM_BLK_RD);
		MEMBER(MPP, EEPROM_BLK_RD_REPLY);
		MEMBER(MPP, DEV_SEND_SEG);
		MEMBER(MPP, DEV_SEND_START);
		MEMBER(MPP, DEV_SEND_END);
		MEMBER(MPP, RESET);
		MEMBER(MPP, HALF_RESET);
		MEMBER(MPP, SER_SEND);
		MEMBER(MPP, SER_RECV);
		/* Twinstar */
		MEMBER(MPP, TWS_WD_MODE_SET);
		MEMBER(MPP, TWS_WD_MODE_GET);
		MEMBER(MPP, TWS_WD_MODE_GET_REPLY);
		MEMBER(MPP, TWS_PORT_SET);
		MEMBER(MPP, TWS_PORT_GET);
		MEMBER(MPP, TWS_PORT_GET_REPLY);
		MEMBER(MPP, TWS_PWR_GET);
		MEMBER(MPP, TWS_PWR_GET_REPLY);
} PACKED members;

/*
 * Statuses
 */
#define	STAT_OK		0x00	/* acknowledges previous command	*/
#define	STAT_FAIL	0x01	/* Last command failed		*/
#define	STAT_RESET_FAIL	0x02	/* reset failed				*/
#define	STAT_NODEST	0x03	/* No destination is selected		*/
#define	STAT_MISMATCH	0x04	/* Data mismatch			*/
#define	STAT_NOACCESS	0x05	/* No access				*/
#define	STAT_BAD_CMD	0x06	/* Bad command				*/
#define	STAT_TOO_SHORT	0x07	/* Packet is too short			*/
#define	STAT_ERROFFS	0x08	/* Offset error				*/
#define	STAT_NOCODE	0x09	/* Source was not burned before		*/
#define	STAT_NO_LEEPROM	0x0A	/* Large EEPROM was not found		*/
#define	STAT_NO_EEPROM	0x0B	/* No EEPROM was found			*/
#define	STAT_WRITE_FAIL	0x0C	/* Writing to device failed		*/
#define	STAT_FPGA_ERR	0x0D	/* FPGA error				*/
#define	STAT_KEY_ERR	0x0E	/* Bad Capabilities Key			*/
#define	STAT_NOCAPS_ERR	0x0F	/* No matching capability		*/
#define	STAT_NOPWR_ERR	0x10	/* No power on USB connector		*/
#define	STAT_CAPS_FPGA_ERR	0x11	/* Setting of the capabilities while FPGA is loaded */

struct xtalk_protocol	mpp_proto = {
	.name	= "MPP",
	.proto_version = 0x14,
	.commands = {
		CMD_SEND(MPP, STATUS_GET),
		CMD_RECV(MPP, STATUS_GET_REPLY),
		CMD_SEND(MPP, EEPROM_SET),
		CMD_SEND(MPP, CAPS_GET),
		CMD_RECV(MPP, CAPS_GET_REPLY),
		CMD_SEND(MPP, CAPS_SET),
		CMD_SEND(MPP, EXTRAINFO_GET),
		CMD_RECV(MPP, EXTRAINFO_GET_REPLY),
		CMD_SEND(MPP, EXTRAINFO_SET),
		CMD_SEND(MPP, RENUM),
		CMD_SEND(MPP, EEPROM_BLK_RD),
		CMD_RECV(MPP, EEPROM_BLK_RD_REPLY),
		CMD_SEND(MPP, DEV_SEND_SEG),
		CMD_SEND(MPP, DEV_SEND_START),
		CMD_SEND(MPP, DEV_SEND_END),
		CMD_SEND(MPP, RESET),
		CMD_SEND(MPP, HALF_RESET),
		CMD_SEND(MPP, SER_SEND),
		CMD_SEND(MPP, SER_RECV),
		/* Twinstar */
		CMD_SEND(MPP, TWS_WD_MODE_SET),
		CMD_SEND(MPP, TWS_WD_MODE_GET),
		CMD_RECV(MPP, TWS_WD_MODE_GET_REPLY),
		CMD_SEND(MPP, TWS_PORT_SET),
		CMD_SEND(MPP, TWS_PORT_GET),
		CMD_RECV(MPP, TWS_PORT_GET_REPLY),
		CMD_SEND(MPP, TWS_PWR_GET),
		CMD_RECV(MPP, TWS_PWR_GET_REPLY),
	},
	.ack_statuses = {
		[STAT_OK] = "Acknowledges previous command",
		[STAT_FAIL] = "Last command failed",
		[STAT_RESET_FAIL] = "Reset failed",
		[STAT_NODEST] = "No destination is selected",
		[STAT_MISMATCH] = "Data mismatch",
		[STAT_NOACCESS] = "No access",
		[STAT_BAD_CMD] = "Bad command",
		[STAT_TOO_SHORT] = "Packet is too short",
		[STAT_ERROFFS] = "Offset error",
		[STAT_NOCODE] = "Source was not burned before",
		[STAT_NO_LEEPROM] = "Large EEPROM was not found",
		[STAT_NO_EEPROM] = "No EEPROM was found",
		[STAT_WRITE_FAIL] = "Writing to device failed",
		[STAT_FPGA_ERR] = "FPGA error",
		[STAT_KEY_ERR] = "Bad Capabilities Key",
		[STAT_NOCAPS_ERR]	= "No matching capability",
		[STAT_NOPWR_ERR]	= "No power on USB connector",
		[STAT_CAPS_FPGA_ERR]	= "Setting of the capabilities while FPGA is loaded",
	}
};

struct mpp_device {
	struct xtalk_base *xtalk_base;
	struct xtalk_sync *xtalk_sync;
	enum eeprom_burn_state burn_state;
	int eeprom_type;
	int status;
	struct firmware_versions fw_versions;
};

struct xusb_iface *xubs_iface_of_mpp(struct mpp_device *mpp)
{
	return xusb_iface_of_xtalk_base(mpp->xtalk_base);
}

struct mpp_device *mpp_new(struct xusb_iface *iface)
{
	struct mpp_device *mpp_dev;
	int ret;

	mpp_dev = calloc(sizeof(*mpp_dev), 1);
	if (!mpp_dev) {
		ERR("Out of memory\n");
		goto err;
	}
	mpp_dev->xtalk_base = xtalk_base_new_on_xusb(iface);
	mpp_dev->xtalk_sync = xtalk_sync_new(mpp_dev->xtalk_base);
	ret = xtalk_sync_set_protocol(mpp_dev->xtalk_sync, &mpp_proto);
        if(ret < 0) {
                ERR("MPP Protocol registration failed: %d\n", ret);
                goto err;
        }
	return mpp_dev;
err:
	if (mpp_dev)
		free(mpp_dev);
	return NULL;
}

void mpp_delete(struct mpp_device *dev)
{
	xtalk_sync_delete(dev->xtalk_sync);
	dev->xtalk_base = NULL;
	free(dev);
}

struct xtalk_sync *xtalk_of_mpp(const struct mpp_device *dev)
{
	return dev->xtalk_sync;
}

struct cmd_queue {
	struct cmd_queue	*next;
	struct cmd_queue	*prev;
	struct xtalk_command	*cmd;
};

static struct cmd_queue	output_queue = {
	.next = &output_queue,
	.prev = &output_queue,
	.cmd = NULL
	};

void dump_command(struct xtalk_command *cmd)
{
	uint16_t	len;
	int		i;

	len = cmd->header.len;
	if(len < sizeof(struct mpp_header)) {
		ERR("Command too short (%d)\n", len);
		return;
	}
	INFO("DUMP: OP=0x%X len=%d seq=%d\n",
		cmd->header.op, cmd->header.len, cmd->header.seq);
	for(i = 0; i < len - sizeof(struct mpp_header); i++) {
		INFO("  %2d. 0x%X\n", i, cmd->alt.raw_data[i]);
	}
}


static int set_ihex_version(char *dst, const char *src)
{
	memcpy(dst, src, VERSION_LEN);
	return 0;
}

enum eeprom_type mpp_eeprom_type(struct mpp_device *mpp_dev)
{
	return mpp_dev->eeprom_type;
}

/*
 * Protocol Commands
 */
int mpp_status_query(struct mpp_device *mpp_dev)
{
	struct xtalk_command *cmd;
	struct xtalk_command *reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, XTALK_OP(MPP, STATUS_GET), 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	mpp_dev->eeprom_type = 0x3 & (CMD_FIELD(reply, MPP, STATUS_GET_REPLY, i2cs_data) >> 3);
	mpp_dev->status = CMD_FIELD(reply, MPP, STATUS_GET_REPLY, status);
	mpp_dev->fw_versions = CMD_FIELD(reply, MPP, STATUS_GET_REPLY, fw_versions);
	DBG("EEPROM TYPE: %02x\n", mpp_dev->eeprom_type);
	DBG("FPGA Firmware: %s\n", (mpp_dev->status & 0x1) ? "Loaded" : "Empty");
	DBG("Firmware Versions: USB='%s' FPGA='%s' EEPROM='%s'\n",
			mpp_dev->fw_versions.usb,
			mpp_dev->fw_versions.fpga,
			mpp_dev->fw_versions.eeprom);
	free_command(reply);
	return ret;
}

int mpp_eeprom_set(struct mpp_device *mpp_dev, const struct eeprom_table *et)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_EEPROM_SET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	memcpy(&CMD_FIELD(cmd, MPP, EEPROM_SET, data), et, sizeof(*et));
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	free_command(reply);
	return 0;
}

int mpp_renumerate(struct mpp_device *mpp_dev)
{
	struct xtalk_command	*cmd;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_RENUM, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	ret = process_command(xtalk_sync, cmd, NULL, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	return 0;
}

int mpp_caps_get(struct mpp_device *mpp_dev,
	struct eeprom_table *eeprom_table,
	struct capabilities *capabilities,
	struct capkey *key)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_CAPS_GET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	assert(reply->header.op == MPP_CAPS_GET_REPLY);
	if(eeprom_table) {
		memcpy(eeprom_table, (void *)&CMD_FIELD(reply, MPP, CAPS_GET_REPLY, data), sizeof(*eeprom_table));
	}
	if(capabilities) {
		const struct capabilities	*cap = &CMD_FIELD(reply, MPP, CAPS_GET_REPLY, capabilities);

		memcpy(capabilities, cap, sizeof(*capabilities));
	}
	if(key) {
		const struct capkey	*k = &CMD_FIELD(reply, MPP, CAPS_GET_REPLY, key);

		memcpy(key, k, sizeof(*key));
	}
	free_command(reply);
	return 0;
}

int mpp_caps_set(struct mpp_device *mpp_dev,
	const struct eeprom_table *eeprom_table,
	const struct capabilities *capabilities,
	const struct capkey *key)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_CAPS_SET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	memcpy(&CMD_FIELD(cmd, MPP, CAPS_SET, data), eeprom_table, sizeof(*eeprom_table));
	memcpy(&CMD_FIELD(cmd, MPP, CAPS_SET, capabilities), capabilities, sizeof(*capabilities));
	memcpy(&CMD_FIELD(cmd, MPP, CAPS_SET, key), key, sizeof(*key));
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	free_command(reply);
	return 0;
}

int mpp_extrainfo_get(struct mpp_device *mpp_dev, struct extrainfo *info)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_EXTRAINFO_GET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	assert(reply->header.op == MPP_EXTRAINFO_GET_REPLY);
	if(info) {
		int i;

		memcpy(info, (void *)&CMD_FIELD(reply, MPP, EXTRAINFO_GET_REPLY, info), sizeof(*info));
		/*
		 * clean non-printing characters
		 */
		for (i = sizeof(*info) - 1; i >= 0; i--) {
			if (info->text[i] != (char)0xFF)
				break;
			info->text[i] = '\0';
		}
	}
	free_command(reply);
	return 0;
}

int mpp_extrainfo_set(struct mpp_device *mpp_dev, const struct extrainfo *info)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_EXTRAINFO_SET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	memcpy(&CMD_FIELD(cmd, MPP, EXTRAINFO_SET, info), info, sizeof(*info));
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	free_command(reply);
	return 0;
}

int mpp_eeprom_blk_rd(struct mpp_device *mpp_dev, uint8_t *buf, uint16_t offset, uint16_t len)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;
	int			size;

	DBG("len = %d, offset = %d\n", len, offset);
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_EEPROM_BLK_RD, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	CMD_FIELD(cmd, MPP, EEPROM_BLK_RD, len) = len;
	CMD_FIELD(cmd, MPP, EEPROM_BLK_RD, offset) = offset;
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		size = ret;
		goto out;
	}
	size = reply->header.len - sizeof(struct mpp_header) - sizeof(XTALK_STRUCT(MPP, EEPROM_BLK_RD_REPLY));
	INFO("size=%d offset=0x%X\n", size, CMD_FIELD(reply, MPP, EEPROM_BLK_RD_REPLY, offset));
	dump_packet(LOG_DEBUG, DBG_MASK, "BLK_RD", (char *)reply, ret);
	if(size > len) {
		ERR("Truncating reply (was %d, now %d)\n", size, len);
		size = len;
	}
	memcpy(buf, CMD_FIELD(reply, MPP, EEPROM_BLK_RD_REPLY, data), size);
out:
	free_command(reply);
	return size;
}

int mpp_send_start(struct mpp_device *mpp_dev, int dest, const char *ihex_version)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply = NULL;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret = 0;

	DBG("dest = %s ihex_version = '%s'\n", dev_dest2str(dest), ihex_version);
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_DEV_SEND_START, 0)) == NULL) {
		ERR("new_command failed\n");
		ret = -ENOMEM;
		goto out;
	}
	CMD_FIELD(cmd, MPP, DEV_SEND_START, dest) = dest;
	set_ihex_version(CMD_FIELD(cmd, MPP, DEV_SEND_START, ihex_version), ihex_version);
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		goto out;
	}
out:
	if(reply)
		free_command(reply);
	mpp_dev->burn_state = (ret > 0)
		? BURN_STATE_STARTED
		: BURN_STATE_FAILED;
	return ret;
}

int mpp_send_end(struct mpp_device *mpp_dev)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply = NULL;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret = 0;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_DEV_SEND_END, 0)) == NULL) {
		ERR("new_command failed\n");
		ret = -ENOMEM;
		goto out;
	}
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		goto out;
	}
out:
	if(reply)
		free_command(reply);
	mpp_dev->burn_state = (ret > 0)
		? BURN_STATE_ENDED
		: BURN_STATE_FAILED;
	return ret;
}

int mpp_send_seg(struct mpp_device *mpp_dev, const uint8_t *data, uint16_t offset, uint16_t len)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if(mpp_dev->burn_state != BURN_STATE_STARTED) {
		ERR("Tried to send a segment while burn_state=%d\n",
				mpp_dev->burn_state);
		return -EINVAL;
	}
	DBG("len = %d, offset = %d (0x%02X, 0x%02X)\n", len, offset, *data, *(data + 1));
	if((cmd = new_command(xtalk_base, MPP_DEV_SEND_SEG, len)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	CMD_FIELD(cmd, MPP, DEV_SEND_SEG, offset) = offset;
	memcpy(CMD_FIELD(cmd, MPP, DEV_SEND_SEG, data), data, len);
#if 0
	{
		FILE			*fp;
		if((fp = fopen("seg_data.bin", "a")) == NULL) {
			perror("seg_data.bin");
			exit(1);
		}
		if(fwrite(CMD_FIELD(cmd, MPP, DEV_SEND_SEG, data), len, 1, fp) != 1) {
			perror("fwrite");
			exit(1);
		}
		fclose(fp);
	}
#endif
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	free_command(reply);
	return 0;
}

int mpp_reset(struct mpp_device *mpp_dev, int full_reset)
{
	struct xtalk_command	*cmd;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;
	int			op = (full_reset) ? MPP_RESET: MPP_HALF_RESET;

	DBG("full = %s\n", (full_reset) ? "YES" : "NO");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, op, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	ret = process_command(xtalk_sync, cmd, NULL, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	return 0;
}

int mpp_serial_cmd(struct mpp_device *mpp_dev, const uint8_t *in, uint8_t *out, uint16_t len)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;
	uint8_t			*data;

	DBG("len=%d\n", len);
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_SER_SEND, len)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	data = CMD_FIELD(cmd, MPP, SER_SEND, data);
	memcpy(data, in, len);
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	assert(reply->header.op == MPP_SER_RECV);
	data = CMD_FIELD(reply, MPP, SER_RECV, data);
	memcpy(out, data, len);
	free_command(reply);
	return 0;
}

int mpps_card_info(struct mpp_device *mpp_dev, int unit, uint8_t *card_type, uint8_t *card_status)
{
	/*
	 * Serial commands must have equal send/receive size
	 */
	struct card_info_command {
		uint8_t	ser_op;
		uint8_t	addr;
		uint8_t	card_full_type;	/* (type << 4 | subtype) */
		uint8_t	card_status;	/* BIT(0) - PIC burned */
	} PACKED;
	struct card_info_command ci_send;
	struct card_info_command ci_recv;
	int ret;

	memset(&ci_send, 0, sizeof(ci_send));
	memset(&ci_recv, 0, sizeof(ci_recv));
	ci_send.ser_op = SER_CARD_INFO_GET;
	ci_send.addr = (unit << 4);	/* low nibble is subunit */
	ret = mpp_serial_cmd(mpp_dev,
		(uint8_t *)&ci_send,
		(uint8_t *)&ci_recv,
		sizeof(struct card_info_command));
	if (ret < 0)
		return ret;
	*card_type = ci_recv.card_full_type;
	*card_status = ci_recv.card_status;
	return 0;
}

int mpps_stat(struct mpp_device *mpp_dev, int unit, uint8_t *fpga_configuration, uint8_t *status)
{
	/*
	 * Serial commands must have equal send/receive size
	 */
	struct fpga_stat_command {
		uint8_t	ser_op;
		uint8_t	fpga_configuration;
		uint8_t	status;	/* BIT(0) - Watchdog timer status */
	} PACKED;
	struct fpga_stat_command fs_send;
	struct fpga_stat_command fs_recv;
	int ret;

	memset(&fs_send, 0, sizeof(fs_send));
	memset(&fs_recv, 0, sizeof(fs_recv));
	fs_send.ser_op = SER_STAT_GET;
	ret = mpp_serial_cmd(mpp_dev,
		(uint8_t *)&fs_send,
		(uint8_t *)&fs_recv,
		sizeof(struct fpga_stat_command));
	if(ret < 0)
		return ret;
	*fpga_configuration = fs_recv.fpga_configuration;
	*status = fs_recv.status;
	return 0;
}

int mpp_tws_watchdog(struct mpp_device *mpp_dev)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_TWS_WD_MODE_GET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	ret = CMD_FIELD(reply, MPP, TWS_WD_MODE_GET_REPLY, wd_active);
	DBG("wd_active=0x%X\n", ret);
	free_command(reply);
	return ret == 1;
}

int mpp_tws_setwatchdog(struct mpp_device *mpp_dev, int yes)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("%s\n", (yes) ? "YES" : "NO");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_TWS_WD_MODE_SET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	CMD_FIELD(cmd, MPP, TWS_WD_MODE_SET, wd_active) = (yes) ? 1 : 0;
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	free_command(reply);
	return 0;
}

int mpp_tws_powerstate(struct mpp_device *mpp_dev)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_TWS_PWR_GET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	ret = CMD_FIELD(reply, MPP, TWS_PWR_GET_REPLY, power);
	DBG("power=0x%X\n", ret);
	free_command(reply);
	return ret;
}

int mpp_tws_portnum(struct mpp_device *mpp_dev)
{
	struct xtalk_command	*cmd;
	struct xtalk_command	*reply;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if((cmd = new_command(xtalk_base, MPP_TWS_PORT_GET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	ret = process_command(xtalk_sync, cmd, &reply, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	ret = CMD_FIELD(reply, MPP, TWS_PORT_GET_REPLY, portnum);
	DBG("portnum=0x%X\n", ret);
	free_command(reply);
	return ret;
}

int mpp_tws_setportnum(struct mpp_device *mpp_dev, uint8_t portnum)
{
	struct xtalk_command	*cmd;
	struct xtalk_sync *xtalk_sync;
	struct xtalk_base *xtalk_base;
	uint16_t tx_seq;
	int			ret;

	DBG("\n");
	assert(mpp_dev != NULL);
	xtalk_sync = mpp_dev->xtalk_sync;
	xtalk_base = mpp_dev->xtalk_base;
	if(portnum >= 2) {
		ERR("Invalid portnum (%d)\n", portnum);
		return -EINVAL;
	}
	if((cmd = new_command(xtalk_base, MPP_TWS_PORT_SET, 0)) == NULL) {
		ERR("new_command failed\n");
		return -ENOMEM;
	}
	CMD_FIELD(cmd, MPP, TWS_PORT_SET, portnum) = portnum;
	ret = process_command(xtalk_sync, cmd, NULL, &tx_seq);
	if(ret < 0) {
		ERR("process_command failed: %d\n", ret);
		return ret;
	}
	return 0;
}

/*
 * data structures
 */

void show_eeprom(const struct eeprom_table *eprm, FILE *fp)
{
	int	rmajor;
	int	rminor;
	char	buf[BUFSIZ];

	rmajor = (eprm->release >> 8) & 0xFF;
	rminor = eprm->release & 0xFF;;
	memset(buf, 0, LABEL_SIZE + 1);
	memcpy(buf, eprm->label, LABEL_SIZE);
	fprintf(fp, "EEPROM: %-15s: 0x%02X\n", "Source", eprm->source);
	fprintf(fp, "EEPROM: %-15s: 0x%04X\n", "Vendor", eprm->vendor);
	fprintf(fp, "EEPROM: %-15s: 0x%04X\n", "Product", eprm->product);
	fprintf(fp, "EEPROM: %-15s: %d.%d\n", "Release", rmajor, rminor);
	fprintf(fp, "EEPROM: %-15s: 0x%02X\n", "Config", eprm->config_byte);
	fprintf(fp, "EEPROM: %-15s: '%s'\n", "Label", buf);
}

void show_capabilities(const struct capabilities *capabilities, FILE *fp)
{
	fprintf(fp, "Capabilities: FXS ports: %2d\n", capabilities->ports_fxs);
	fprintf(fp, "Capabilities: FXO ports: %2d\n", capabilities->ports_fxo);
	fprintf(fp, "Capabilities: BRI ports: %2d\n", capabilities->ports_bri);
	fprintf(fp, "Capabilities: PRI ports: %2d\n", capabilities->ports_pri);
	fprintf(fp, "Capabilities: ECHO ports: %2d\n", capabilities->ports_echo);
	fprintf(fp, "Capabilities: TwinStar : %s\n",
		(CAP_EXTRA_TWINSTAR(capabilities)) ? "Yes" : "No");
}

void show_astribank_status(struct mpp_device *mpp_dev, FILE *fp)
{
	char	version_buf[BUFSIZ];
	int	is_loaded = STATUS_FPGA_LOADED(mpp_dev->status);

	fprintf(fp, "Astribank: EEPROM      : %s\n",
		eeprom_type2str(mpp_dev->eeprom_type));
	fprintf(fp, "Astribank: FPGA status : %s\n",
		is_loaded ? "Loaded" : "Empty");
	if(is_loaded) {
		memset(version_buf, 0, sizeof(version_buf));
		memcpy(version_buf, mpp_dev->fw_versions.fpga, VERSION_LEN);
		fprintf(fp, "Astribank: FPGA version: %s\n",
			version_buf);
	}
}

void show_extrainfo(const struct extrainfo *extrainfo, FILE *fp)
{
	char	buf[EXTRAINFO_SIZE + 1];

	memcpy(buf, extrainfo->text, EXTRAINFO_SIZE);
	buf[EXTRAINFO_SIZE] = '\0';	/* assure null termination */
	fprintf(fp, "Extrainfo:             : '%s'\n", buf);
}

int twinstar_show(struct mpp_device *mpp, FILE *fp)
{
	int	watchdog;
	int	powerstate;
	int	portnum;
	int	i;

	if((watchdog = mpp_tws_watchdog(mpp)) < 0) {
		ERR("Failed getting TwinStar information\n");
		return watchdog;
	}
	if((powerstate = mpp_tws_powerstate(mpp)) < 0) {
		ERR("Failed getting TwinStar powerstate\n");
		return powerstate;
	}
	if((portnum = mpp_tws_portnum(mpp)) < 0) {
		ERR("Failed getting TwinStar portnum\n");
		return portnum;
	}
	fprintf(fp, "TwinStar: Connected to : USB-%1d\n", portnum);
	fprintf(fp, "TwinStar: Watchdog     : %s\n",
		(watchdog) ? "on-guard" : "off-guard");
	for(i = 0; i < 2; i++) {
		int	pw = (1 << i) & powerstate;

		fprintf(fp, "TwinStar: USB-%1d POWER  : %s\n",
			i, (pw) ? "ON" : "OFF");
	}
	return 0;
}

int show_hardware(struct mpp_device *mpp_dev)
{
	int	ret;
	struct eeprom_table	eeprom_table;
	struct capabilities	capabilities;
	struct extrainfo	extrainfo;

	ret = mpp_caps_get(mpp_dev, &eeprom_table, &capabilities, NULL);
	if(ret < 0)
		return ret;
	show_eeprom(&eeprom_table, stdout);
	show_astribank_status(mpp_dev, stdout);
	if(mpp_dev->eeprom_type == EEPROM_TYPE_LARGE) {
		show_capabilities(&capabilities, stdout);
		if(STATUS_FPGA_LOADED(mpp_dev->status)) {
			uint8_t	unit;
			uint8_t	card_status;
			uint8_t	card_type;
			uint8_t	fpga_configuration;
			uint8_t	status;

			for(unit = 0; unit < 5; unit++) {
				ret = mpps_card_info(mpp_dev, unit, &card_type, &card_status);
				if(ret < 0)
					return ret;
				printf("CARD %d: type=%x.%x %s\n", unit,
						((card_type >> 4) & 0xF), (card_type & 0xF),
						((card_status & 0x1) ? "PIC" : "NOPIC"));
			}
			ret = mpps_stat(mpp_dev, unit, &fpga_configuration, &status);
			if (ret < 0)
				return ret;
			printf("FPGA: %-17s: %d\n", "Configuration num", fpga_configuration);
			printf("FPGA: %-17s: %s\n", "Watchdog Timer",
				(SER_STAT_WATCHDOG_READY(status)) ? "ready" : "expired");
			printf("FPGA: %-17s: %s\n", "XPD Alive",
				(SER_STAT_XPD_ALIVE(status)) ? "yes" : "no");
		}
		ret = mpp_extrainfo_get(mpp_dev, &extrainfo);
		if(ret < 0)
			return ret;
		show_extrainfo(&extrainfo, stdout);
		if(CAP_EXTRA_TWINSTAR(&capabilities)) {
			if ((eeprom_table.product & 0xFFF0) != 0x1160)
				printf("TwinStar: NO\n");
			else
				twinstar_show(mpp_dev, stdout);
		}
	}
	return 0;
}

