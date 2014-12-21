
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
