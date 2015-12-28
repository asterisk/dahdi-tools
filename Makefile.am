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

SUBDIRS	= xpp doc hotplug

if PPPD
SUBDIRS	+= ppp
endif

noinst_HEADERS	= \
	bittest.h	\
	dahdi_tools_version.h	\
	fxotune.h	\
	wavformat.h	\
	#

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

dist_sbin_SCRIPTS	= \
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
dahdiinclude_HEADERS	= tonezone.h
libtonezone_la_CFLAGS	= $(CFLAGS) -I$(srcdir) -DBUILDING_TONEZONE
libtonezone_la_LDFLAGS	= -version-info "$(LTZ_CURRENT):$(LTZ_REVISION):$(LTZ_AGE)"
libtonezone_la_LIBADD	= -lm

if PBX_PCAP
noinst_PROGRAMS		+= dahdi_pcap
dahdi_pcap_LDADD	= -lpcap
endif

patlooptest_LDADD	= libtonezone.la
fxstest_LDADD		= libtonezone.la
fxotune_LDADD		= -lm
dahdi_speed_CFLAGS	= -O2

dahdi_maint_SOURCES	= dahdi_maint.c version.c

if PBX_NEWT
sbin_PROGRAMS		+= dahdi_tool
dahdi_tool_CFLAGS	= $(CFLAGS) $(NEWT_INCLUDE)
dahdi_tool_LDADD	= $(NEWT_LIB)
endif

dahdi_cfg_LDFLAGS	= -lpthread
dahdi_cfg_LDADD		= libtonezone.la

udevrulesdir	= @udevrulesdir@
udevrules_DATA	= dahdi.rules

all-local:
	$(LEGACY_MAKE)  all

clean-local:
	$(LEGACY_MAKE) clean

install-exec-hook:
	$(LEGACY_MAKE) install
	@echo "Compatibility symlinks (should be removed in the future)"
	ln -sf libtonezone.so.2.0.0 $(DESTDIR)$(libdir)/libtonezone.so.2.0

bashcompdir	= $(sysconfdir)/bash_completion.d

install-data-hook:
	mkdir -p $(DESTDIR)$(bashcompdir)
	install -m 644 $(srcdir)/dahdi-bash-completion $(DESTDIR)$(bashcompdir)/dahdi

# Handle these files specially -- install them only with 'install-config'
special_config_files	= \
	init.conf.sample	\
	blacklist.sample	\
	modprobe.conf.sample	\
	#

install-config:
	mkdir -p $(DESTDIR)$(dahdiconfdir)
	mkdir -p $(DESTDIR)$(sysconfdir)/modprobe.d
	install -m644 xpp/genconf_parameters $(DESTDIR)$(dahdiconfdir)/genconf_parameters
	install -m644 init.conf.sample $(DESTDIR)$(dahdiconfdir)/init.conf
	install -m644 blacklist.sample $(DESTDIR)$(sysconfdir)/modprobe.d/dahdi-blacklist.conf
	install -m644 modprobe.conf.sample $(DESTDIR)$(sysconfdir)/modprobe.d/dahdi.conf
	$(LEGACY_MAKE) config

dahdiconfdir	= @sysconfdir@/dahdi
dahdiconf_DATA	= \
	system.conf.sample	\
	assigned-spans.conf.sample	\
	span-types.conf.sample	\
	modules.sample	\
	#

build_scripts	= \
	build_tools/dahdi_svn_tarball	\
	build_tools/dahdi_sysfs_copy	\
	build_tools/dump_sys_state	\
	build_tools/make_firmware_object.in	\
	build_tools/make_tree	\
	build_tools/make_version	\
	build_tools/test_kernel_git	\
	build_tools/uninstall-modules	\
	#

.version:
	if ./build_tools/make_version . dahdi/tools > $@.tmp; then \
		mv "$@.tmp" "$@"; \
	fi

uninstall-hook:
	$(RM) $(DESTDIR)$(sysconfdir)/dahdi/span-types.conf.sample
	$(RM) $(DESTDIR)$(sysconfdir)/dahdi/assigned-spans.conf.sample
	$(RM) $(DESTDIR)$(sysconfdir)/dahdi/system.conf
	$(RM) $(DESTDIR)$(bashcompdir)/dahdi

docs:
	$(LEGACY_MAKE)  $@

DISTCLEANFILES	= makeopts config.log config.status .*.d

MAINTAINERCLEANFILES	= \
		m4/libtool.m4 \
		m4/ltoptions.m4 \
		m4/ltsugar.m4 \
		m4/ltversion.m4 \
		m4/lt~obsolete.m4 \
		#

EXTRA_DIST	= \
	.version	\
	LICENSE	\
	LICENSE.LGPL	\
	UPGRADE.txt	\
	bootstrap.sh	\
	Makefile.legacy	\
	dahdi.init	\
	dahdi.xml	\
	dahdi_pcap.c	\
	ifup-hdlc	\
	dahdi-bash-completion	\
	$(special_config_files)	\
	$(dahdiconf_DATA)	\
	$(udevrules_DATA)	\
	$(build_scripts)	\
	#

.PHONY: docs config
