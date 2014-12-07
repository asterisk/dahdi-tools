ACLOCAL_AMFLAGS	= -I m4

LEGACY_MAKE	= \
	 $(MAKE) -f $(srcdir)/Makefile.legacy \
		top_srcdir=$(top_srcdir) \
		srcdir=$(srcdir)

SUBDIRS	= xpp

all-local:
	$(LEGACY_MAKE)  all

clean-local:
	$(LEGACY_MAKE) clean

install-exec-hook:
	$(LEGACY_MAKE) install

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
