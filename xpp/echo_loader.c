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

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <regex.h>
#include <sys/time.h>
#include <unistd.h>
#include <oct6100api/oct6100_api.h>
#include <xtalk/debug.h>
#include <xtalk/xusb.h>
#include <astribank.h>
#include "echo_loader.h"
#include "parse_span_specs.h"

#ifdef	__GNUC__
#define	PACKED	__attribute__((packed))
#else
#define	PACKED
#endif

#define DBG_MASK        	0x10
#define	TIMEOUT			1000
#define ECHO_MAX_CHANS		128
#define ECHO_RIN_STREAM		0
#define ECHO_ROUT_STREAM	1
#define ECHO_SIN_STREAM		2
#define ECHO_SOUT_STREAM	3

#define ECHO_RIN_STREAM2 	4
#define ECHO_SIN_STREAM2 	6
#define ECHO_ROUT_STREAM2 	5
#define ECHO_SOUT_STREAM2 	7

#define EC_VER_TEST		0xABCD
#define EC_VER_INVALID		0xFFFF
static float oct_fw_load_timeout = 2.0;

struct echo_mod {
	tPOCT6100_INSTANCE_API pApiInstance;
	UINT32 ulEchoChanHndl[256];
	struct astribank *astribank;
	int maxchans;
};

enum xpp_packet_types {
	SPI_SND_XOP 	= 0x0F,
	SPI_RCV_XOP 	= 0x10,
	TST_SND_XOP 	= 0x35,
	TST_RCV_XOP 	= 0x36,
};

struct xpp_packet_header {
	struct {
		uint16_t	len;
		uint8_t		op;
		uint8_t		unit;
	} PACKED header;
	union {
		struct {
			uint8_t		header;
			uint8_t		flags;
			uint8_t		addr_l;
			uint8_t		addr_h;
			uint8_t		data_l;
			uint8_t		data_h;
		} PACKED spi_pack;
		struct {
			uint8_t		tid;
			uint8_t		tsid;
		} PACKED tst_pack;
	} alt;
} PACKED;

static struct usb_buffer {
	char	data[PACKET_SIZE];
	int	max_len;
	int	curr;
	/* statistics */
	int	min_send;
	int	max_send;
	int	num_sends;
	long	total_bytes;
	struct timeval	start;
	struct timeval	end;
} usb_buffer;


static void usb_buffer_init(struct astribank *astribank, struct usb_buffer *ub)
{
	ub->max_len = xusb_packet_size(xusb_dev_of_astribank(astribank));
	ub->curr = 0;
	ub->min_send = INT_MAX;
	ub->max_send = 0;
	ub->num_sends = 0;
	ub->total_bytes = 0;
	gettimeofday(&ub->start, NULL);
}

static long usb_buffer_usec(struct usb_buffer *ub)
{
	struct timeval	now;

	gettimeofday(&now, NULL);
	return (now.tv_sec - ub->start.tv_sec) * 1000000 +
		(now.tv_usec - ub->start.tv_usec);
}

static void usb_buffer_showstatistics(struct astribank *astribank, struct usb_buffer *ub)
{
	long	usec;

	usec = usb_buffer_usec(ub);
	AB_INFO(astribank, "Octasic statistics: packet_size=[%d, %ld, %d] packets=%d, bytes=%ld msec=%ld usec/packet=%ld\n",
		ub->min_send,
		ub->total_bytes / ub->num_sends,
		ub->max_send,
		ub->num_sends, ub->total_bytes,
		usec / 1000, usec / ub->num_sends);
}

static int usb_buffer_flush(struct astribank *astribank, struct usb_buffer *ub)
{
	int	ret;
	long	t;
	long	sec;
	static int	last_sec;

	if (ub->curr == 0)
		return 0;
	ret = astribank_send(astribank, 0, ub->data, ub->curr, TIMEOUT);
	if (ret < 0) {
		AB_ERR(astribank, "xusb_send failed: %d\n", ret);
		return ret;
	}
	DBG("%s: Written %d bytes\n", __func__, ret);
	if (ret > ub->max_send)
		ub->max_send = ret;
	if (ret < ub->min_send)
		ub->min_send = ret;
	ub->total_bytes += ret;
	ub->num_sends++;
	ub->curr = 0;

	sec = usb_buffer_usec(ub) / (1000 * 1000);
	if (sec > last_sec) {
		DBG("bytes/sec=%ld average len=%ld\n",
			ub->total_bytes / sec,
			ub->total_bytes / ub->num_sends);
		last_sec = sec;
	}

	/*
	 * Best result with high frequency firmware: 21 seconds
	 * Octasic statistics: packet_size=[10, 239, 510] packets=26806, bytes=6419640 usec=21127883 usec/packet=788
	 * t = 0.3 * ret - 150;
	 */
	t = oct_fw_load_timeout * ret - 150;
	if (t > 0)
		usleep(t);
	return ret;
}

static int usb_buffer_append(struct astribank *astribank, struct usb_buffer *ub,
	char *buf, int len)
{
	if (ub->curr + len >= ub->max_len) {
		AB_ERR(astribank, "%s: buffer too small ub->curr=%d, len=%d, ub->max_len=%d\n",
			__func__, ub->curr, len, ub->max_len);
		return -ENOMEM;
	}
	memcpy(ub->data + ub->curr, buf, len);
	ub->curr += len;
	return len;
}

static int usb_buffer_send(struct astribank *astribank, struct usb_buffer *ub,
	char *buf, int len, int timeout, int recv_answer)
{
	int	ret = 0;

	if (ub->curr + len >= ub->max_len) {
		ret = usb_buffer_flush(astribank, ub);
		if (ret < 0)
			return ret;
	}

	if ((ret = usb_buffer_append(astribank, ub, buf, len)) < 0) {
		return ret;
	}
	DBG("%s: %d bytes %s\n", __func__, len, (recv_answer) ? "recv" : "send");
	if (recv_answer) {
		struct xpp_packet_header	*phead;

		ret = usb_buffer_flush(astribank, ub);
		if (ret < 0)
			return ret;
		ret = astribank_recv(astribank, 0, buf, PACKET_SIZE, TIMEOUT);
		if (ret <= 0) {
			AB_ERR(astribank, "No USB packs to read: %s\n", strerror(-ret));
			return -EINVAL;
		}
		DBG("%s: %d bytes recv\n", __func__, ret);
		phead = (struct xpp_packet_header *)buf;
		if (phead->header.op != SPI_RCV_XOP &&
			phead->header.op != TST_RCV_XOP) {
			AB_ERR(astribank, "Got unexpected reply OP=0x%02X\n",
				phead->header.op);
			dump_packet(LOG_ERR, DBG_MASK, "hexline[ERR]",
				buf, ret);
			return -EINVAL;
		}
		dump_packet(LOG_DEBUG, DBG_MASK, "dump:echoline[R]", (char *)phead, phead->header.len);
		switch(phead->header.op) {
		case SPI_RCV_XOP:
			ret = (phead->alt.spi_pack.data_h << 8) | phead->alt.spi_pack.data_l;
			break;
		case TST_RCV_XOP:
			ret = (phead->alt.tst_pack.tid << 8) | phead->alt.tst_pack.tsid;
			break;
		default:
			ret = -EINVAL;
		}
	}
	return ret;
}

int spi_send(struct astribank *astribank, uint16_t addr, uint16_t data, int recv_answer, int ver)
{
	int				ret;
	char				buf[PACKET_SIZE];
	struct xpp_packet_header	*phead = (struct xpp_packet_header *)buf;
	int				pack_len;
	int				spi_flags;

	assert(astribank != NULL);
	spi_flags = 0x30 | (recv_answer ? 0x40 : 0x00) | (ver ? 0x01 : 0x00);
	pack_len = sizeof(phead->header) + sizeof(phead->alt.spi_pack);
	phead->header.len 		= pack_len;
	phead->header.op 		= SPI_SND_XOP;
	phead->header.unit 		= 0x40;	/* EC has always this unit num */
	phead->alt.spi_pack.header 	= 0x05;
	phead->alt.spi_pack.flags 	= spi_flags;
	phead->alt.spi_pack.addr_l	= (addr >> 0) & 0xFF;
	phead->alt.spi_pack.addr_h	= (addr >> 8) & 0xFF;
	phead->alt.spi_pack.data_l	= (data >> 0) & 0xFF;
	phead->alt.spi_pack.data_h	= (data >> 8) & 0xFF;

	dump_packet(LOG_DEBUG, DBG_MASK, "dump:echoline[W]", (char *)phead, pack_len);


	ret = usb_buffer_send(astribank, &usb_buffer, buf, pack_len, TIMEOUT, recv_answer);
	if (ret < 0) {
		AB_ERR(astribank, "usb_buffer_send failed: %d\n", ret);
		return ret;
	}
	DBG("%s: Written %d bytes\n", __func__, ret);
	return ret;
}

int test_send(struct astribank *astribank)
{
	int                             ret;
	char                            buf[PACKET_SIZE];
	struct xpp_packet_header       *phead = (struct xpp_packet_header *)buf;
	int                             pack_len;

	assert(astribank != NULL);
	pack_len = sizeof(phead->header) + sizeof(phead->alt.tst_pack);
	phead->header.len               = 6;
	phead->header.op                = 0x35;
	phead->header.unit              = 0x00;
	phead->alt.tst_pack.tid         = 0x28; /* EC TestId	*/
	phead->alt.tst_pack.tsid        = 0x00; /* EC SubId	*/

	dump_packet(LOG_DEBUG, DBG_MASK, "dump:echoline[W]",
		(char *)phead, pack_len);

	ret = usb_buffer_send(astribank,
		&usb_buffer, buf, pack_len, TIMEOUT, 1);
	if (ret < 0) {
		AB_ERR(astribank, "usb_buffer_send failed: %d\n", ret);
		return ret;
	}
	DBG("%s: Written %d bytes\n", __func__, ret);
	return ret;
}

int echo_send_data(struct astribank *astribank, const unsigned int addr, const unsigned int data)
{
	int ret;
/*	DBG("SEND: %04X -> [%04X]\n", data, addr);
	DBG("\t\t[%04X] <- %04X\n", 0x0008, (addr >> 20));
	DBG("\t\t[%04X] <- %04X\n", 0x000A, (addr >> 4) & ((1 << 16) - 1));
	DBG("\t\t[%04X] <- %04X\n", 0x0004, data);
	DBG("\t\t[%04X] <- %04X\n", 0x0000, (((addr >> 1) & 0x7) << 9) | (1 << 8) | (3 << 12) | 1);
 */

	DBG("SND:\n");
	ret = spi_send(astribank, 0x0008, (addr >> 20)			, 0, 0);
	if (ret < 0)
		goto failed;
	ret = spi_send(astribank, 0x000A, (addr >> 4) & ((1 << 16) - 1)	, 0, 0);
	if (ret < 0)
		goto failed;
	ret = spi_send(astribank, 0x0004, data				, 0, 0);
	if (ret < 0)
		goto failed;
	ret = spi_send(astribank, 0x0000, (((addr >> 1) & 0x7) << 9) |
				(1 << 8) | (3 << 12) | 1		, 0, 0);
	if (ret < 0)
		goto failed;
	return cOCT6100_ERR_OK;
failed:
	AB_ERR(astribank, "echo_send_data: spi_send failed (ret = %d)\n", ret);
	return ret;
}

int echo_recv_data(struct astribank *astribank, const unsigned int addr)
{
	unsigned int data = 0x00;
	int ret;

	DBG("RCV:\n");
	ret = spi_send(astribank, 0x0008, (addr >> 20)			, 0, 0);
	if (ret < 0)
		goto failed;
	ret = spi_send(astribank, 0x000A, (addr >> 4) & ((1 << 16) - 1)	, 0, 0);
	if (ret < 0)
		goto failed;
	ret = spi_send(astribank, 0x0000, (((addr >> 1) & 0x7) << 9) |
				(1 << 8) | 1				, 0, 0);
	if (ret < 0)
		goto failed;
	ret = spi_send(astribank, 0x0004, data				, 1, 0);
	if (ret < 0)
		goto failed;
	return ret;
failed:
	AB_ERR(astribank, "echo_recv_data: spi_send failed (ret = %d)\n", ret);
	return ret;
}

int load_file(char *filename, unsigned char **ppBuf, UINT32 *pLen)
{
	unsigned char *pbyFileData = NULL;
	FILE *pFile;

	DBG("Loading %s file...\n", filename);
	pFile = fopen(filename, "rb");
	if (pFile == NULL) {
		ERR("fopen: %s\n", strerror(errno));
		return -ENODEV;
	}

	fseek(pFile, 0L, SEEK_END);
	*pLen = ftell(pFile);
	fseek(pFile, 0L, SEEK_SET);

	pbyFileData = (unsigned char *)malloc(*pLen);
	if (pbyFileData == NULL) {
		fclose(pFile);
		ERR("malloc\n");
		return -ENODEV;
	} else {
		DBG("allocated mem for pbyFileData\n");
	}
	if (fread(pbyFileData, 1, *pLen, pFile) != *pLen) {
		fclose(pFile);
		ERR("fread: %s\n", strerror(errno));
		return -ENODEV;
	}
	fclose(pFile);
	DBG("Successful loading %s file into memory "
		"(size = %d, DUMP: first = %02X %02X, last = %02X %02X)\n",
			filename, *pLen,
			pbyFileData[0], pbyFileData[1],
			pbyFileData[(*pLen)-2], pbyFileData[(*pLen)-1]);
	*ppBuf = pbyFileData;
	return 0;
}

UINT32 Oct6100UserGetTime(tPOCT6100_GET_TIME f_pTime)
{
	///* Why couldn't they just take a timeval like everyone else? */
	struct timeval tv;
	unsigned long long total_usecs;
	unsigned int mask = ~0;

	gettimeofday(&tv, 0);
	total_usecs = (((unsigned long long)(tv.tv_sec)) * 1000000) +
				  (((unsigned long long)(tv.tv_usec)));
	f_pTime->aulWallTimeUs[0] = (total_usecs & mask);
	f_pTime->aulWallTimeUs[1] = (total_usecs >> 32);
	//printf("Inside of Oct6100UserGetTime\n");
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserMemSet(PVOID f_pAddress, UINT32 f_ulPattern, UINT32 f_ulLength)
{
	memset(f_pAddress, f_ulPattern, f_ulLength);
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserMemCopy(PVOID f_pDestination, const void *f_pSource, UINT32 f_ulLength)
{
	memcpy(f_pDestination, f_pSource, f_ulLength);
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserCreateSerializeObject(tPOCT6100_CREATE_SERIALIZE_OBJECT f_pCreate)
{
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserDestroySerializeObject(tPOCT6100_DESTROY_SERIALIZE_OBJECT f_pDestroy)
{
#ifdef OCTASIC_DEBUG
	ERR("I should never be called! (destroy serialize object)\n");
#endif
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserSeizeSerializeObject(tPOCT6100_SEIZE_SERIALIZE_OBJECT f_pSeize)
{
	/* Not needed */
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserReleaseSerializeObject(tPOCT6100_RELEASE_SERIALIZE_OBJECT f_pRelease)
{
	/* Not needed */
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserDriverWriteApi(tPOCT6100_WRITE_PARAMS f_pWriteParams)
{
	const unsigned int 		addr 		= f_pWriteParams->ulWriteAddress;
	const unsigned int 		data 		= f_pWriteParams->usWriteData;
	const struct echo_mod		*echo_mod 	= (struct echo_mod *)(f_pWriteParams->pProcessContext);
	struct astribank 	*astribank 	= echo_mod->astribank;
	int ret;

	ret = echo_send_data(astribank, addr, data);
	if (ret < 0) {
		ERR("echo_send_data failed (ret = %d)\n", ret);
		return cOCT6100_ERR_FATAL_DRIVER_WRITE_API;
	}
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserDriverWriteSmearApi(tPOCT6100_WRITE_SMEAR_PARAMS f_pSmearParams)
{
	unsigned int              	addr;
	unsigned int              	data;
	unsigned int              	len;
	const struct echo_mod           *echo_mod;
	struct astribank   	*astribank;
	unsigned int 			i;

	len = f_pSmearParams->ulWriteLength;
	echo_mod = (struct echo_mod *)f_pSmearParams->pProcessContext;
	astribank = echo_mod->astribank;
	for (i = 0; i < len; i++) {
		int ret;

		addr = f_pSmearParams->ulWriteAddress + (i << 1);
		data = f_pSmearParams->usWriteData;
		ret = echo_send_data(astribank, addr, data);
		if (ret < 0) {
			ERR("echo_send_data failed (ret = %d)\n", ret);
			return cOCT6100_ERR_FATAL_DRIVER_WRITE_API;
		}
	}
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserDriverWriteBurstApi(tPOCT6100_WRITE_BURST_PARAMS f_pBurstParams)
{
	unsigned int              	addr;
	unsigned int              	data;
	unsigned int 			len 		= f_pBurstParams->ulWriteLength;
	const struct echo_mod		*echo_mod 	= (struct echo_mod *)f_pBurstParams->pProcessContext;
	struct astribank 	*astribank 	= echo_mod->astribank;
	unsigned int 			i;

	for (i = 0; i < len; i++) {
		int ret;

		addr = f_pBurstParams->ulWriteAddress + (i << 1);
		data = f_pBurstParams->pusWriteData[i];
		ret = echo_send_data(astribank, addr, data);
		if (ret < 0) {
			ERR("echo_send_data failed (ret = %d)\n", ret);
			return cOCT6100_ERR_FATAL_DRIVER_WRITE_API;
		}
	}
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserDriverReadApi(tPOCT6100_READ_PARAMS f_pReadParams)
{
	const unsigned int              addr =  f_pReadParams->ulReadAddress;
	const struct echo_mod		*echo_mod;
	struct astribank 	*astribank;
	int ret;

	echo_mod = (struct echo_mod *)f_pReadParams->pProcessContext;
	astribank = echo_mod->astribank;
	ret = echo_recv_data(astribank, addr);
	if (ret < 0) {
		ERR("echo_recv_data failed (%d)\n", ret);
		return cOCT6100_ERR_FATAL_DRIVER_READ_API;
	}
	*f_pReadParams->pusReadData = ret;
	return cOCT6100_ERR_OK;
}

UINT32 Oct6100UserDriverReadBurstApi(tPOCT6100_READ_BURST_PARAMS f_pBurstParams)
{
	unsigned int              	addr;
	unsigned int              	len;
	const struct echo_mod		*echo_mod;
	struct astribank 	*astribank;
	unsigned int 			i;

	len = f_pBurstParams->ulReadLength;
	echo_mod = (struct echo_mod *)f_pBurstParams->pProcessContext;
	astribank = echo_mod->astribank;
	for (i = 0;i < len; i++) {
		unsigned int ret;

		addr = f_pBurstParams->ulReadAddress + (i << 1);
		ret = echo_recv_data(astribank, addr);
		if (ret < 0) {
			ERR("echo_recv_data failed (%d)\n", ret);
			return cOCT6100_ERR_FATAL_DRIVER_READ_API;
		}
		f_pBurstParams->pusReadData[i] = ret;
	}
	return cOCT6100_ERR_OK;
}

static inline int get_ver(struct astribank *astribank)
{
	return spi_send(astribank, 0, 0, 1, 1);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
UINT32 init_octasic(char *filename, struct astribank *astribank, struct span_specs *span_specs)
{
	int							cpld_ver;
	struct echo_mod						*echo_mod;
	UINT32							nChan;
	UINT32							nSlot;
	UINT32							pcmLaw;
	UINT32							ulResult;

	tOCT6100_GET_INSTANCE_SIZE 				InstanceSize;
	tPOCT6100_INSTANCE_API 					pApiInstance;
	tOCT6100_CHIP_OPEN					OpenChip;

	UINT32							ulImageByteSize;
	PUINT8							pbyImageData = NULL;

	/*=========================================================================*/
	/* Channel resources.*/
	tOCT6100_CHANNEL_OPEN					ChannelOpen;
	UINT32							ulChanHndl;
	enum tdm_codec tdm_codec;
	int spanno;

	if (test_send(astribank) < 0)
		return cOCT6100_ERR_FATAL;
	cpld_ver = get_ver(astribank);
	AB_INFO(astribank, "Check EC_CPLD version: %d\n", cpld_ver);
	if (cpld_ver < 0)
		return cOCT6100_ERR_FATAL;
	else if (cpld_ver == EC_VER_TEST) {
		AB_INFO(astribank, "+---------------------------------------------------------+\n");
		AB_INFO(astribank, "| WARNING: TEST HARDWARE IS ON THE BOARD INSTEAD OF EC!!! |\n");
		AB_INFO(astribank, "+---------------------------------------------------------+\n");
		return cOCT6100_ERR_OK;
	}


	/**************************************************************************/
	/**************************************************************************/
	/*	1) Configure and Open the OCT6100.			          */
	/**************************************************************************/
	/**************************************************************************/

	memset(&InstanceSize, 0, sizeof(tOCT6100_GET_INSTANCE_SIZE));
	memset(&OpenChip, 0, sizeof(tOCT6100_CHIP_OPEN));

	echo_mod = malloc(sizeof(struct echo_mod));
	if (!echo_mod) {
		AB_ERR(astribank, "cannot allocate memory for echo_mod\n");
		return cOCT6100_ERR_FATAL;
	}
	DBG("allocated mem for echo_mod\n");

	memset(echo_mod, 0, sizeof(struct echo_mod));

	/* Fill the OCT6100 Chip Open configuration structure with default values */

	ulResult = Oct6100ChipOpenDef(&OpenChip);
	if (ulResult != cOCT6100_ERR_OK) {
		AB_ERR(astribank, "Oct6100ChipOpenDef failed: result=%X\n",
			ulResult);
		return ulResult;
	}

	OpenChip.pProcessContext      			= echo_mod;
	/* Configure clocks */

	/* upclk oscillator is at 33.33 Mhz */
	OpenChip.ulUpclkFreq 				= cOCT6100_UPCLK_FREQ_33_33_MHZ;

	/* mclk will be generated by internal PLL at 133 Mhz */
	OpenChip.fEnableMemClkOut 			= TRUE;
	OpenChip.ulMemClkFreq 				= cOCT6100_MCLK_FREQ_133_MHZ;

	/* General parameters */
	OpenChip.fEnableChannelRecording 		= TRUE;

	/* Chip ID.*/
	OpenChip.ulUserChipId 				= 1;

	/* Set the max number of accesses to 1024 to speed things up */
	/* OpenChip.ulMaxRwAccesses 			= 1024;      */

	/* Set the maximums that the chip needs to support for this test */
	OpenChip.ulMaxChannels				= 256;
	OpenChip.ulMaxPlayoutBuffers			= 2;

	OpenChip.ulMaxBiDirChannels			= 0;
	OpenChip.ulMaxConfBridges			= 0;
	OpenChip.ulMaxPhasingTssts			= 0;
	OpenChip.ulMaxTdmStreams			= 8;
	OpenChip.ulMaxTsiCncts				= 0;

	/* External Memory Settings: Use DDR memory*/
	OpenChip.ulMemoryType				= cOCT6100_MEM_TYPE_DDR;

	OpenChip.ulNumMemoryChips			= 1;
	OpenChip.ulMemoryChipSize			= cOCT6100_MEMORY_CHIP_SIZE_32MB;


	/* Load the image file */
	ulResult = load_file(	filename,
			&pbyImageData,
			&ulImageByteSize);

	if (ulResult != 0) {
		AB_ERR(astribank, "Failed load_file %s (%08X)\n", filename, ulResult);
		return ulResult;
	}
	if (pbyImageData == NULL || ulImageByteSize == 0){
		AB_ERR(astribank, "Bad pbyImageData or ulImageByteSize\n");
		return cOCT6100_ERR_FATAL;
	}

	/* Assign the image file.*/
	OpenChip.pbyImageFile				= pbyImageData;
	OpenChip.ulImageSize				= ulImageByteSize;

	/*
	 * Inserting default values into tOCT6100_GET_INSTANCE_SIZE
	 * structure parameters.
	 */
	Oct6100GetInstanceSizeDef(&InstanceSize);

	/* Get the size of the OCT6100 instance structure. */
	ulResult = Oct6100GetInstanceSize(&OpenChip, &InstanceSize);
	if (ulResult != cOCT6100_ERR_OK) {
		AB_ERR(astribank, "Oct6100GetInstanceSize failed (%08X)\n",
			ulResult);
		return ulResult;
	}

	pApiInstance = malloc(InstanceSize.ulApiInstanceSize);
	echo_mod->pApiInstance 				= pApiInstance;
	echo_mod->astribank 				= astribank;

	if (!pApiInstance) {
		AB_ERR(astribank, "Out of memory (can't allocate %d bytes)!\n",
			InstanceSize.ulApiInstanceSize);
		return cOCT6100_ERR_FATAL;
	}

	/* Perform actual open of chip */
	ulResult = Oct6100ChipOpen(pApiInstance, &OpenChip);
	if (ulResult != cOCT6100_ERR_OK) {
		AB_ERR(astribank, "Oct6100ChipOpen failed: result=%X\n",
			ulResult);
		return ulResult;
	}
	DBG("%s: OCT6100 is open\n", __func__);

	/* Free the image file data  */
	free(pbyImageData);

	/**************************************************************************/
	/**************************************************************************/
	/*	2) Open channels in echo cancellation mode.                       */
	/**************************************************************************/
	/**************************************************************************/

	for (nChan = 0; nChan < ECHO_MAX_CHANS; nChan++) {
		nSlot 						= nChan;
		/* open a channel.*/
		Oct6100ChannelOpenDef(&ChannelOpen);

		/* Assign the handle memory.*/
		ChannelOpen.pulChannelHndl = &ulChanHndl;

		/* Set the channel to work at the echo cancellation mode.*/
		ChannelOpen.ulEchoOperationMode 	= cOCT6100_ECHO_OP_MODE_NORMAL;

		spanno = nChan % 4;
		assert(spanno >= 0 && spanno < MAX_SPANNO);
		tdm_codec = span_specs->span_is_alaw[spanno];
		if (tdm_codec == TDM_CODEC_UNKNOWN) {
			AB_ERR(astribank, "Calculated bad alaw/ulaw on channel %d\n", nChan);
			return cOCT6100_ERR_FATAL;
		}
		if (nChan < 4)
			AB_INFO(astribank, "ECHO PRI port %d = %s\n", spanno+1, (tdm_codec == TDM_CODEC_ALAW) ? "alaw" : "ulaw");

		pcmLaw						= ((tdm_codec == TDM_CODEC_ALAW) ? cOCT6100_PCM_A_LAW: cOCT6100_PCM_U_LAW);

		/* Configure the TDM interface.*/
		ChannelOpen.TdmConfig.ulRinPcmLaw		= pcmLaw;
		ChannelOpen.TdmConfig.ulRinStream		= ECHO_RIN_STREAM;
		ChannelOpen.TdmConfig.ulRinTimeslot		= nSlot;

		ChannelOpen.TdmConfig.ulSinPcmLaw		= pcmLaw;
		ChannelOpen.TdmConfig.ulSinStream		= ECHO_SIN_STREAM;
		ChannelOpen.TdmConfig.ulSinTimeslot		= nSlot;

		ChannelOpen.TdmConfig.ulRoutPcmLaw		= pcmLaw;
		ChannelOpen.TdmConfig.ulRoutStream		= ECHO_ROUT_STREAM;
		ChannelOpen.TdmConfig.ulRoutTimeslot		= nSlot;

		ChannelOpen.TdmConfig.ulSoutPcmLaw		= pcmLaw;
		ChannelOpen.TdmConfig.ulSoutStream		= ECHO_SOUT_STREAM;
		ChannelOpen.TdmConfig.ulSoutTimeslot		= nSlot;

		/* Set the desired VQE features.*/
		ChannelOpen.VqeConfig.fEnableNlp		= TRUE;
		ChannelOpen.VqeConfig.fRinDcOffsetRemoval	= TRUE;
		ChannelOpen.VqeConfig.fSinDcOffsetRemoval	= TRUE;

		ChannelOpen.VqeConfig.ulComfortNoiseMode	= cOCT6100_COMFORT_NOISE_NORMAL;
		/*        cOCT6100_COMFORT_NOISE_NORMAL
			  cOCT6100_COMFORT_NOISE_EXTENDED,
			  cOCT6100_COMFORT_NOISE_OFF,
			  cOCT6100_COMFORT_NOISE_FAST_LATCH
		 */
		ulResult = Oct6100ChannelOpen(	pApiInstance,
				&ChannelOpen);
		if (ulResult != cOCT6100_ERR_OK) {
			AB_ERR(astribank, "Found error on chan %d\n", nChan);
			return ulResult;
		}
	}

	/**************************************************************************/
	/**************************************************************************/
	/*	*) Open channels in echo cancellation mode for second bus.        */
	/**************************************************************************/
	/**************************************************************************/

	for (nChan = 8; nChan < 32; nChan++) {
		nSlot 						= (nChan >> 3) * 32 + (nChan & 0x07);
		/* open a channel.*/
		Oct6100ChannelOpenDef(&ChannelOpen);

		/* Assign the handle memory.*/
		ChannelOpen.pulChannelHndl = &ulChanHndl;

		/* Set the channel to work at the echo cancellation mode.*/
		ChannelOpen.ulEchoOperationMode 	= cOCT6100_ECHO_OP_MODE_NORMAL;

		/* Configure the TDM interface.*/
		ChannelOpen.TdmConfig.ulRinStream		= ECHO_RIN_STREAM2;;
		ChannelOpen.TdmConfig.ulRinTimeslot		= nSlot;

		ChannelOpen.TdmConfig.ulSinStream		= ECHO_SIN_STREAM2;
		ChannelOpen.TdmConfig.ulSinTimeslot		= nSlot;

		ChannelOpen.TdmConfig.ulRoutStream		= ECHO_ROUT_STREAM2;
		ChannelOpen.TdmConfig.ulRoutTimeslot		= nSlot;

		ChannelOpen.TdmConfig.ulSoutStream		= ECHO_SOUT_STREAM2;
		ChannelOpen.TdmConfig.ulSoutTimeslot		= nSlot;

		/* Set the desired VQE features.*/
		ChannelOpen.VqeConfig.fEnableNlp		= TRUE;
		ChannelOpen.VqeConfig.fRinDcOffsetRemoval	= TRUE;
		ChannelOpen.VqeConfig.fSinDcOffsetRemoval	= TRUE;

		ChannelOpen.VqeConfig.ulComfortNoiseMode	= cOCT6100_COMFORT_NOISE_NORMAL;
		/*        cOCT6100_COMFORT_NOISE_NORMAL
			  cOCT6100_COMFORT_NOISE_EXTENDED,
			  cOCT6100_COMFORT_NOISE_OFF,
			  cOCT6100_COMFORT_NOISE_FAST_LATCH
		 */
		ulResult = Oct6100ChannelOpen(	pApiInstance,
				&ChannelOpen);
		if (ulResult != cOCT6100_ERR_OK) {
			AB_ERR(astribank, "Found error on chan %d\n", nChan);
			return ulResult;
		}
	}


	DBG("%s: Finishing\n", __func__);
	free(pApiInstance);
	free(echo_mod);
	return cOCT6100_ERR_OK;

}
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int load_echo(struct astribank *astribank, char *filename, int default_is_alaw, const char *span_spec)
{
	int		ret;
	UINT32		octasic_status;
	struct span_specs *span_specs;

	span_specs = parse_span_specifications(span_spec, default_is_alaw);
	if (!span_specs) {
		AB_ERR(astribank, "ECHO parsing span specs failed\n");
		return -EFAULT;
	}
	AB_INFO(astribank, "Loading ECHOCAN Firmware: %s (default %s)\n",
		filename, (default_is_alaw) ? "alaw" : "ulaw");
	usb_buffer_init(astribank, &usb_buffer);
	octasic_status = init_octasic(filename, astribank, span_specs);
	free_span_specifications(span_specs);
	if (octasic_status != cOCT6100_ERR_OK) {
		AB_ERR(astribank, "ECHO %s burning failed (%08X)\n",
			filename, octasic_status);
		return -ENODEV;
	}
	ret = usb_buffer_flush(astribank, &usb_buffer);
	if (ret < 0) {
		AB_ERR(astribank, "ECHO %s buffer flush failed (%d)\n", filename, ret);
		return -ENODEV;
	}
	usb_buffer_showstatistics(astribank, &usb_buffer);
	return 0;
}

int echo_ver(struct astribank *astribank)
{
	usb_buffer_init(astribank, &usb_buffer);
	return get_ver(astribank);
}

