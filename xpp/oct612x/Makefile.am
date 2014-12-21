GLOBAL_CFLAGS	= -Wall -Werror

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

noinst_HEADERS	= \
		apilib/bt/octapi_bt0_private.h	\
		apilib/llman/octapi_llman_private.h	\
		include/apilib/octapi_bt0.h	\
		include/apilib/octapi_largmath.h	\
		include/apilib/octapi_llman.h	\
		include/digium_unused.h	\
		include/oct6100api/oct6100_adpcm_chan_inst.h	\
		include/oct6100api/oct6100_adpcm_chan_pub.h	\
		include/oct6100api/oct6100_api.h	\
		include/oct6100api/oct6100_api_inst.h	\
		include/oct6100api/oct6100_apimi.h	\
		include/oct6100api/oct6100_apiud.h	\
		include/oct6100api/oct6100_channel_inst.h	\
		include/oct6100api/oct6100_channel_pub.h	\
		include/oct6100api/oct6100_chip_open_inst.h	\
		include/oct6100api/oct6100_chip_open_pub.h	\
		include/oct6100api/oct6100_chip_stats_inst.h	\
		include/oct6100api/oct6100_chip_stats_pub.h	\
		include/oct6100api/oct6100_conf_bridge_inst.h	\
		include/oct6100api/oct6100_conf_bridge_pub.h	\
		include/oct6100api/oct6100_debug_inst.h	\
		include/oct6100api/oct6100_debug_pub.h	\
		include/oct6100api/oct6100_defines.h	\
		include/oct6100api/oct6100_errors.h	\
		include/oct6100api/oct6100_events_inst.h	\
		include/oct6100api/oct6100_events_pub.h	\
		include/oct6100api/oct6100_interrupts_inst.h	\
		include/oct6100api/oct6100_interrupts_pub.h	\
		include/oct6100api/oct6100_mixer_inst.h	\
		include/oct6100api/oct6100_mixer_pub.h	\
		include/oct6100api/oct6100_phasing_tsst_inst.h	\
		include/oct6100api/oct6100_phasing_tsst_pub.h	\
		include/oct6100api/oct6100_playout_buf_inst.h	\
		include/oct6100api/oct6100_playout_buf_pub.h	\
		include/oct6100api/oct6100_remote_debug_inst.h	\
		include/oct6100api/oct6100_remote_debug_pub.h	\
		include/oct6100api/oct6100_tlv_inst.h	\
		include/oct6100api/oct6100_tone_detection_inst.h	\
		include/oct6100api/oct6100_tone_detection_pub.h	\
		include/oct6100api/oct6100_tsi_cnct_inst.h	\
		include/oct6100api/oct6100_tsi_cnct_pub.h	\
		include/oct6100api/oct6100_tsst_inst.h	\
		include/octdef.h	\
		include/octmac.h	\
		include/octosdependant.h	\
		include/octrpc/oct6100_rpc_protocol.h	\
		include/octrpc/rpc_protocol.h	\
		include/octtype.h	\
		include/octtypevx.h	\
		include/octtypewin.h	\
		#

liboctasic_la_CFLAGS	= \
	$(GLOBAL_CFLAGS) \
	$(OCTASIC_DEFINES) \
	$(OCTASIC_CFLAGS)


EXTRA_DIST	= \
		get_discards	\
		octasic-helper	\
		octdeviceapi/oct6100api/oct6100_adpcm_chan_priv.h	\
		octdeviceapi/oct6100api/oct6100_api/oct6100_user.c	\
		octdeviceapi/oct6100api/oct6100_apimi/oct6100_mask_interrupts.c	\
		octdeviceapi/oct6100api/oct6100_channel_priv.h	\
		octdeviceapi/oct6100api/oct6100_chip_open_priv.h	\
		octdeviceapi/oct6100api/oct6100_chip_stats_priv.h	\
		octdeviceapi/oct6100api/oct6100_conf_bridge_priv.h	\
		octdeviceapi/oct6100api/oct6100_debug_priv.h	\
		octdeviceapi/oct6100api/oct6100_events_priv.h	\
		octdeviceapi/oct6100api/oct6100_interrupts_priv.h	\
		octdeviceapi/oct6100api/oct6100_memory_priv.h	\
		octdeviceapi/oct6100api/oct6100_miscellaneous_priv.h	\
		octdeviceapi/oct6100api/oct6100_mixer_priv.h	\
		octdeviceapi/oct6100api/oct6100_phasing_tsst_priv.h	\
		octdeviceapi/oct6100api/oct6100_playout_buf_priv.h	\
		octdeviceapi/oct6100api/oct6100_remote_debug_priv.h	\
		octdeviceapi/oct6100api/oct6100_tlv_priv.h	\
		octdeviceapi/oct6100api/oct6100_tone_detection_priv.h	\
		octdeviceapi/oct6100api/oct6100_tsi_cnct_priv.h	\
		octdeviceapi/oct6100api/oct6100_tsst_priv.h	\
		octdeviceapi/oct6100api/oct6100_version.h	\
		test.c	\
		#
