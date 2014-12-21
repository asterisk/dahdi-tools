OCTASIC_DEFINES	= \
	-DPTR_TYPE=uint32_t	\
	-DcOCT6100_INTERNAL_SUPER_ARRAY_SIZE=1024	\
	-DcOCT6100_MAX_ECHO_CHANNELS=672		\
	-DcOCT6100_MAX_MIXER_EVENTS=1344

noinst_LTLIBRARIES	= liboctasic.la

# Taken from octasic-helper
liboctasic_la_SOURCES	= \
	octdeviceapi/oct6100api/oct6100_api/oct6100_adpcm_chan.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_channel.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_chip_open.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_chip_stats.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_conf_bridge.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_debug.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_events.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_interrupts.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_memory.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_miscellaneous.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_mixer.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_phasing_tsst.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_playout_buf.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_remote_debug.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_tlv.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_tone_detection.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_tsi_cnct.c \
	octdeviceapi/oct6100api/oct6100_api/oct6100_tsst.c \
	apilib/bt/octapi_bt0.c \
	apilib/largmath/octapi_largmath.c \
	apilib/llman/octapi_llman.c

liboctasic_la_CFLAGS	= \
	$(OCTASIC_DEFINES) \
	$(OCTASIC_CFLAGS)
