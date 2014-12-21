#ifndef	MPPTALK_H
#define	MPPTALK_H
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

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * MPPTALK - Example XTALK dialect
 */

#include <xtalk/proto.h>
#include <xtalk/proto_sync.h>

#ifdef	__GNUC__
#define	PACKED	__attribute__((packed))
#else
#define	PACKED
#endif

/*---------------- Common types --------------------*/

/*
 * The eeprom_table is common to all eeprom types.
 */
#define	LABEL_SIZE	8
struct eeprom_table {
	uint8_t		source;		/* C0 - small eeprom, C2 - large eeprom */
	uint16_t	vendor;
	uint16_t	product;
	uint16_t	release;	/* BCD encoded release */
	uint8_t		config_byte;	/* Must be 0 */
	uint8_t		label[LABEL_SIZE];
} PACKED;

#define	VERSION_LEN	6
struct firmware_versions {
	char	usb[VERSION_LEN];
	char	fpga[VERSION_LEN];
	char	eeprom[VERSION_LEN];
} PACKED;

struct capabilities {
	uint8_t		ports_fxs;
	uint8_t		ports_fxo;
	uint8_t		ports_bri;
	uint8_t		ports_pri;
	uint8_t		extra_features;	/* BIT(0) - TwinStar */
	uint8_t		ports_echo;
	uint8_t		reserved[2];
	uint32_t	timestamp;
} PACKED;

#define	CAP_EXTRA_TWINSTAR(c)		((c)->extra_features & 0x01)
#define	CAP_EXTRA_TWINSTAR_SET(c)	do {(c)->extra_features |= 0x01;} while (0)
#define	CAP_EXTRA_TWINSTAR_CLR(c)	do {(c)->extra_features &= ~0x01;} while (0)

#define	KEYSIZE	16
struct capkey {
	uint8_t	k[KEYSIZE];
} PACKED;

#define	EXTRAINFO_SIZE	24
struct extrainfo {
	char		text[EXTRAINFO_SIZE];
} PACKED;

struct mpp_header {
	uint16_t	len;
	uint16_t	seq;
	uint8_t		op;	/* MSB: 0 - to device, 1 - from device */
} PACKED;

enum mpp_ser_op {
	SER_CARD_INFO_GET	= 0x1,
	SER_STAT_GET		= 0x3,
/* Status bits */
#define	SER_STAT_WATCHDOG_READY(s)	((s) & 0x01)
#define	SER_STAT_XPD_ALIVE(s)		((s) & 0x02)
};

/* EEPROM_QUERY: i2cs(ID1, ID0) */
enum eeprom_type {
	EEPROM_TYPE_NONE	= 0,
	EEPROM_TYPE_SMALL	= 1,
	EEPROM_TYPE_LARGE	= 2,
	EEPROM_TYPE_UNUSED	= 3,
};

enum dev_dest {
	DEST_NONE	= 0x00,
	DEST_FPGA	= 0x01,
	DEST_EEPROM	= 0x02,
};


/*---------------- PROTOCOL ------------------------*/
/* API */
struct mpp_device;

struct mpp_device *mpp_new(struct xusb_iface *iface);
void mpp_delete(struct mpp_device *dev);
struct xusb_iface *xubs_iface_of_mpp(struct mpp_device *mpp);
int mpp_status_query(struct mpp_device *mpp_dev);

enum eeprom_type mpp_eeprom_type(struct mpp_device *mpp_dev);

void show_eeprom(const struct eeprom_table *eprm, FILE *fp);
void show_capabilities(const struct capabilities *capabilities, FILE *fp);
void show_astribank_status(struct mpp_device *mpp_dev, FILE *fp);
void show_extrainfo(const struct extrainfo *extrainfo, FILE *fp);
int twinstar_show(struct mpp_device *mpp, FILE *fp);
int show_hardware(struct mpp_device *mpp_dev);

int mpp_renumerate(struct mpp_device *mpp_dev);
int mpp_send_start(struct mpp_device *mpp_dev, int dest, const char *ihex_version);
int mpp_send_end(struct mpp_device *mpp_dev);
int mpp_send_seg(struct mpp_device *mpp_dev, const uint8_t *data, uint16_t offset, uint16_t len);
int mpp_reset(struct mpp_device *mpp_dev, int full_reset);

int mpp_caps_get(struct mpp_device *mpp_dev,
	struct eeprom_table *eeprom_table,
	struct capabilities *capabilities,
	struct capkey *key);
int mpp_caps_set(struct mpp_device *mpp_dev,
	const struct eeprom_table *eeprom_table,
	const struct capabilities *capabilities,
	const struct capkey *key);

/*
 * serial sub-protocol to FPGA
 */
int mpps_card_info(struct mpp_device *mpp, int unit, uint8_t *card_type, uint8_t *card_status);
int mpps_stat(struct mpp_device *mpp, int unit, uint8_t *maincard_version, uint8_t *status);

/*
 * Twinstar
 */
int mpp_tws_watchdog(struct mpp_device *mpp);
int mpp_tws_setwatchdog(struct mpp_device *mpp, int yes);
int mpp_tws_powerstate(struct mpp_device *mpp);
int mpp_tws_portnum(struct mpp_device *mpp);
int mpp_tws_setportnum(struct mpp_device *mpp, uint8_t portnum);

const char *dev_dest2str(int dest);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	/* MPPTALK_H */
