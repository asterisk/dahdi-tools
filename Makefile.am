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

.PHONY: docs config
