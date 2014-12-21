ACLOCAL_AMFLAGS	= -I m4

LEGACY_MAKE	= \
	 $(MAKE) -f $(srcdir)/Makefile.legacy \
		top_srcdir=$(top_srcdir) \
		srcdir=$(srcdir)

CFLAGS	= -g -Wall -O2 $(DAHDI_INCLUDE)
if DAHDI_DEVMODE
CFLAGS	+= \
	-Werror \
	-Wunused \
	-Wundef \
	$(DAHDI_DECLARATION_AFTER_STATEMENT) \
	-Wmissing-format-attribute \
	-Wformat-security \
	#-Wformat=2
endif

SUBDIRS	= xpp doc

if PPPD
SUBDIRS	+= ppp
endif

sbin_PROGRAMS	= \
	dahdi_test \
	dahdi_maint \
	dahdi_monitor \
	dahdi_cfg \
	dahdi_speed \
	dahdi_scan \
	fxotune

noinst_PROGRAMS	= \
	fxstest \
	patgen \
	pattest \
	patlooptest \
	dahdi_diag \
	timertest

sbin_SCRIPTS	= \
	dahdi_span_assignments \
	dahdi_waitfor_span_assignments \
	dahdi_span_types

if PBX_HDLC
sbin_PROGRAMS	+= sethdlc
noinst_PROGRAMS += hdlcstress hdlctest hdlcgen hdlcverify
endif

# Libtool versioning for libtonezone:
# Bump when interface changes
LTZ_CURRENT	= 2
# Bump if interface change is backward compatible
LTZ_AGE		= 0
# Bump if only implementation change
LTZ_REVISION	= 0

lib_LTLIBRARIES		= libtonezone.la

libtonezone_la_SOURCES	= \
	zonedata.c \
	tonezone.c \
	version.c
pkginclude_HEADERS	= tonezone.h
libtonezone_la_CFLAGS	= $(CFLAGS) -I$(srcdir) -DBUILDING_TONEZONE
libtonezone_la_LDFLAGS	= -version-info "$(LTZ_CURRENT):$(LTZ_REVISION):$(LTZ_AGE)"

patlooptest_LDADD	= libtonezone.la -lm
fxstest_LDADD		= libtonezone.la -lm
fxotune_LDADD		= -lm
dahdi_speed_CFLAGS	= -O2

dahdi_maint_SOURCES	= dahdi_maint.c version.c

if PBX_NEWT
sbin_PROGRAMS		+= dahdi_tool
dahdi_tool_CFLAGS	= $(CFLAGS) $(NEWT_INCLUDE)
dahdi_tool_LDADD	= $(NEWT_LIB)
endif

dahdi_cfg_LDFLAGS	= -lm -lpthread
dahdi_cfg_LDADD		= libtonezone.la

all-local:
	$(LEGACY_MAKE)  all

clean-local:
	$(LEGACY_MAKE) clean

install-exec-hook:
	$(LEGACY_MAKE) install
	@echo "Compatibility symlinks (should be removed in the future)"
	ln -s libtonezone.so.2.0.0 $(DESTDIR)$(libdir)/libtonezone.so.2.0

dist:
	$(LEGACY_MAKE) dist

docs config:
	$(LEGACY_MAKE)  $@

DISTCLEANFILES	= makeopts config.log config.status .*.d

MAINTAINERCLEANFILES	= \
		m4/libtool.m4 \
		m4/ltoptions.m4 \
		m4/ltsugar.m4 \
		m4/ltversion.m4 \
		m4/lt~obsolete.m4 \
		#

.PHONY: docs config
