
all-local: $(perl_checks)

SUBDIRS		=

man_MANS	=


# FIXME: try to improve code, so we can use $(PEDANTIC)
#PEDANTIC	= -ansi -pedantic -std=c99
GLOBAL_CFLAGS	= -I$(srcdir) -I$(srcdir)/xtalk $(PEDANTIC)

if DAHDI_DEVMODE
GLOBAL_CFLAGS	+= \
	-Werror \
	-Wunused \
	-Wundef \
	-Wmissing-format-attribute \
	-Wformat-security \
	-Wformat=2
endif

if PERL
SUBDIRS		+= perl_modules

%.8: %
	@if file "$^" | cut -d: -f2 | grep -q perl; then \
		if pod2man --section 8 $^ > $@; then \
			echo "  GEN      $@"; \
		else \
			rm -f "$@"; \
		fi \
	fi

%.check: %
	@echo "  CHECK    $^"; \
	if ! perl -I./perl_modules -c $^ 2>/dev/null; then \
		perl -I./perl_modules -c $^; \
	fi
	@touch $@

perl_scripts	=	\
		dahdi_registration	\
		xpp_sync	\
		lsdahdi		\
		xpp_blink	\
		dahdi_genconf	\
		dahdi_hardware	\
		twinstar	\
		#

perl_checks	= $(perl_scripts:%=%.check)
perl_mans	= $(perl_scripts:%=%.8)

endif

dist_dahditools_SCRIPTS	= xpp_fxloader astribank_hook waitfor_xpds
udevrulesdir	= @udevrulesdir@
udevrules_DATA	= xpp.rules

dist_sbin_SCRIPTS	= $(perl_scripts)
man_MANS		+= $(perl_mans)
CLEANFILES		= $(perl_checks) $(perl_mans)

if	PBX_USB

SUBDIRS		+= oct612x

noinst_LTLIBRARIES	= libastribank.la libecholoader.la libhexfile.la

libastribank_la_SOURCES	= \
		astribank_usb.c	\
		astribank_usb.h	\
		mpptalk.c	\
		mpptalk.h	\
		mpp.h	\
		mpptalk_defs.h	\
		xtalk/debug.c	\
		xtalk/debug.h	\
		xtalk/xlist.c	\
		xtalk/xlist.h	\
		xtalk/xtalk.c	\
		xtalk/xtalk.h	\
		xtalk/xtalk_defs.h	\
		xtalk/xusb.c	\
		xtalk/xusb.h	\
		#

libastribank_la_CFLAGS		= $(GLOBAL_CFLAGS)

if USE_OCTASIC
libecholoader_la_SOURCES	= \
		parse_span_specs.c \
		parse_span_specs.h	\
		echo_loader.c	\
		echo_loader.h	\
		#

libecholoader_la_CFLAGS		= $(GLOBAL_CFLAGS) $(OCTASIC_CFLAGS)
endif

libhexfile_la_SOURCES	= \
		hexfile.c	\
		hexfile.h	\
		#

sbin_PROGRAMS	= \
		astribank_tool \
		astribank_hexload \
		astribank_allow \
		astribank_is_starting

check_PROGRAMS		= test_parse
test_parse_LDADD	= libhexfile.la

astribank_tool_SOURCES		= astribank_tool.c
astribank_tool_CFLAGS		= $(GLOBAL_CFLAGS)
astribank_tool_LDFLAGS		= $(USB_LIB)
astribank_tool_LDADD		= libastribank.la

astribank_hexload_SOURCES	= \
			astribank_hexload.c	\
			pic_loader.c	\
			pic_loader.h	\
			#

astribank_hexload_CFLAGS	= $(GLOBAL_CFLAGS) $(OCTASIC_CFLAGS)
astribank_hexload_LDFLAGS	= $(USB_LIB)
astribank_hexload_LDADD		= \
		libhexfile.la	\
		libastribank.la	\
		libecholoader.la	\
		oct612x/liboctasic.la	\
		#

astribank_allow_SOURCES		= \
			astribank_allow.c	\
			astribank_license.c	\
			astribank_license.h	\
			#

astribank_allow_CFLAGS		= $(GLOBAL_CFLAGS)
astribank_allow_LDFLAGS		= $(USB_LIB)
astribank_allow_LDADD		= libastribank.la

man_pages	= \
		astribank_tool.8 \
		astribank_hexload.8 \
		astribank_allow.8 \
		astribank_is_starting.8

man_MANS	+= $(man_pages)
endif

EXTRA_DIST	= \
	$(man_pages)	\
	$(udevrules_DATA)	\
	README.Astribank	\
	astribank_upgrade	\
	dahdi.cgi	\
	dahdi_drivers	\
	genconf_parameters	\
	twinstar_hook	\
	twinstar_setup	\
	xpp_modprobe	\
	xpp_timing	\
	#

