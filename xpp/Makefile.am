
LEGACY_MAKE	= \
	 $(MAKE) -f $(srcdir)/Makefile.legacy \
		top_srcdir=$(top_srcdir) \
		srcdir=$(srcdir)

all-local:
	$(LEGACY_MAKE)  all

clean-local:
	$(LEGACY_MAKE)  clean

install-exec-hook:
	$(LEGACY_MAKE)  install

# Cannot allow this yet
distclean:
maintainer-clean:

SUBDIRS		=

GLOBAL_CFLAGS	= -I$(srcdir) -I$(srcdir)/xtalk

if	PBX_USB

SUBDIRS		+= oct612x

noinst_LTLIBRARIES	= libastribank.la libecholoader.la
libastribank_la_SOURCES	= \
		xtalk/xusb.c \
		xtalk/xlist.c \
		xtalk/debug.c \
		xtalk/xtalk.c \
		mpptalk.c \
		astribank_usb.c
libastribank_la_CFLAGS		= $(GLOBAL_CFLAGS)

if USE_OCTASIC
libecholoader_la_SOURCES	= \
		parse_span_specs.c \
		echo_loader.c
libecholoader_la_CFLAGS		= $(GLOBAL_CFLAGS) $(OCTASIC_CFLAGS)
endif

sbin_PROGRAMS	= \
		astribank_tool \
		astribank_hexload \
		astribank_allow \
		astribank_is_starting

check_PROGRAMS	= test_parse

astribank_tool_SOURCES		= astribank_tool.c
astribank_tool_CFLAGS		= $(GLOBAL_CFLAGS)
astribank_tool_LDFLAGS		= $(USB_LIB)
astribank_tool_LDADD		= libastribank.la

astribank_hexload_SOURCES	= astribank_hexload.c hexfile.c pic_loader.c
astribank_hexload_CFLAGS	= $(GLOBAL_CFLAGS) $(OCTASIC_CFLAGS)
astribank_hexload_LDFLAGS	= $(USB_LIB)
astribank_hexload_LDADD		= libastribank.la libecholoader.la oct612x/liboctasic.la

astribank_allow_SOURCES		= astribank_allow.c astribank_license.c
astribank_allow_CFLAGS		= $(GLOBAL_CFLAGS)
astribank_allow_LDFLAGS		= $(USB_LIB)
astribank_allow_LDADD		= libastribank.la

man_MANS	= \
		astribank_tool.8 \
		astribank_hexload.8 \
		astribank_allow.8 \
		astribank_is_starting.8
endif
