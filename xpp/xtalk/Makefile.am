VISIBILITY_DEFS		= -DXTALK_DLL -DXTALK_DLL_EXPORTS
COMMON_CFLAGS		= \
		-Wall \
		-Wno-unknown-pragmas \
		-Werror \
		$(VISIBILITY_DEFS) \
		$(CFLAG_VISIBILITY)

AM_CFLAGS		= $(COMMON_CFLAGS)


if	LIBUSBX
USB_CFLAGS		= $(LIBUSBX_CFLAGS)
USB_LIBS		= $(LIBUSBX_LIBS)
USB_NAME		= libusbx
else
if	LIBUSB
USB_CFLAGS		= $(LIBUSB_CFLAGS)
USB_LIBS		= $(LIBUSB_LIBS)
USB_NAME		= libusb
endif
endif

noinst_PROGRAMS		= xlist_test xusb_test xusb_test_bypath xtalk_test xtalk_raw_test
sbin_PROGRAMS		= xtalk_send
noinst_LTLIBRARIES	= libxtalk.la
dist_noinst_HEADERS	= \
	xtalk_base.h	\
	xusb_common.h	\
	include/xtalk/proto_raw.h	\
	include/xtalk/api_defs.h	\
	include/xtalk/xlist.h	\
	include/xtalk/proto_sync.h	\
	include/xtalk/xusb_iface.h	\
	include/xtalk/proto.h	\
	include/xtalk/debug.h	\
	include/xtalk/xusb.h	\
	include/xtalk/firmware_defs.h	\
	include/xtalk/xtalk_iface.h	\
	#

man_MANS		= xtalk_send.8

libxtalk_la_CFLAGS	= \
		$(COMMON_CFLAGS) \
		-I$(srcdir)/include \
		-I$(srcdir) \
		$(USB_CFLAGS) \
		-DXTALK_OPTIONS_FILE=\"/etc/dahdi/xpp.conf\"

libxtalk_la_LDFLAGS	= #
libxtalk_la_LIBADD	= $(USB_LIBS)
libxtalk_la_SOURCES	= \
			$(dist_noinst_HEADERS) \
			xtalk_sync.c \
			xtalk_raw.c \
			xtalk_base.c \
			xlist.c \
			debug.c \
			xtalk-xusb.c	\
			xusb_common.c
if	LIBUSBX
libxtalk_la_SOURCES	+= xusb_libusbx.c
else
if	LIBUSB
libxtalk_la_SOURCES	+= xusb_libusb.c
endif
endif
libxtalk_la_DEPENDENCIES	= $(libxtalk_la_SOURCES)

xtalk_send_CFLAGS	= $(COMMON_CFLAGS) -I$(srcdir)/include -I$(srcdir)
xtalk_send_LDADD	= libxtalk.la $(USB_LIBS)

xtalk_test_CFLAGS	= $(COMMON_CFLAGS) -I$(srcdir)/include -I$(srcdir)
xtalk_test_LDADD	= libxtalk.la $(USB_LIBS)

xtalk_raw_test_CFLAGS	= $(COMMON_CFLAGS) -I$(srcdir)/include -I$(srcdir)
xtalk_raw_test_LDADD	= libxtalk.la $(USB_LIBS)

xusb_test_CFLAGS	= $(COMMON_CFLAGS) -I$(srcdir)/include -I$(srcdir)
xusb_test_LDADD		= libxtalk.la $(USB_LIBS)

xusb_test_bypath_CFLAGS	= $(COMMON_CFLAGS) -I$(srcdir)/include -I$(srcdir)
xusb_test_bypath_LDADD		= libxtalk.la $(USB_LIBS)

xlist_test_CFLAGS	= $(COMMON_CFLAGS) -I$(srcdir)/include -I$(srcdir)
xlist_test_LDADD	= libxtalk.la $(USB_LIBS)

DISTCLEANFILES		= xtalk.pc xtalk-uninstalled.pc

EXTRA_DIST	= $(man_MANS)
