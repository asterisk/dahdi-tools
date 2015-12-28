# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
 
m4_define([short_version],
          m4_bpatsubst(m4_esyscmd([build_tools/make_version . dahdi/tools]),
                       [\([0-9.]*\)\(\w\|\W\)*],
                       [\1]))
AC_INIT([dahdi-tools], short_version, [www.asterisk.org])

# check existence of the package
AC_CONFIG_SRCDIR([dahdi_cfg.c])
AC_CONFIG_AUX_DIR([auxdir])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign subdir-objects])

build_version=`build_tools/make_version . dahdi/tools`
AC_SUBST([BUILD_VERSION], "$build_version")

# enable nice build output on automake1.11
m4_pattern_allow([AM_DEFAULT_VERBOSITY])
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],[AM_DEFAULT_VERBOSITY=1])
AM_CONDITIONAL([SILENT_RULES], [test "$AM_DEFAULT_VERBOSITY" = '0'])

AC_COPYRIGHT("dahdi-tools")
AC_REVISION($Revision$)

# Preserve old defaults if called without --prefix or --prefix=/usr
#  prefix=/usr
#  sysconfdir=/etc

ac_default_prefix=/usr
if test "$prefix" = 'NONE' -o "$prefix" = '/usr'; then
	if test ${sysconfdir} = '${prefix}/etc'; then
	   sysconfdir=/etc
	fi
	if test ${mandir} = '${prefix}/man'; then
	   mandir=/usr/share/man
	fi

	if test ${localstatedir} = '${prefix}/var'; then
	     localstatedir=/var
	fi
fi
AC_SUBST([dahditoolsdir], ['${datadir}/dahdi'])
AC_SUBST([dahdiincludedir], ['${includedir}/dahdi'])

# Where to install udev rules?
#  * Our default is $sysconfdir/udev/rules.d:
#    - This preserve legacy location.
#    - It doesn't write into modern vendor supplied location (/lib/udev/rules.d)
#    - It doesn't break "make distcheck" (where everything should be relative
#      to prefix.
#  * Packagers are encouraged to use --udevrules=/lib/udev/rules.d
#
m4_define([_UDEV_RULES_DIR_DEFAULT], [${sysconfdir}/udev/rules.d])dnl
AC_ARG_WITH(udevrules,
	[AS_HELP_STRING([--with-udevrules=PATH],
		[Location of UDEV rules @<:@default=]_UDEV_RULES_DIR_DEFAULT[@:>@])],
	[udevrulesdir=$withval],
	[udevrulesdir=]'_UDEV_RULES_DIR_DEFAULT'
)
AC_SUBST([udevrulesdir], "$udevrulesdir")

# specify output header file
AC_CONFIG_HEADER(autoconfig.h)

# This needs to be before any macros that use the C compiler
AC_GNU_SOURCE

AC_CHECK_TOOL([LD], [ld])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PROG_LIBTOOL
AST_CHECK_GNU_MAKE

test_obj=conftest.o
AC_COMPILE_IFELSE(AC_LANG_SOURCE(),[
	BDFNAME=`LANG=C objdump -f $test_obj | grep -e "$test_obj:" | sed "s/.*file format \(.*\)/\1/"`
	BDFARCH=`LANG=C objdump -f $test_obj | grep -e "architecture:" | sed "s/.*ture: \(.*\),.*/\1/"`	   
],[])
AC_SUBST(BDFNAME)
AC_SUBST(BDFARCH)

# Set the default value of HOSTCC from CC if --host was not provided:
HOSTCC=${HOSTCC:=${CC}}
AC_SUBST(HOSTCC)

AC_PATH_PROG([GREP], [grep], :)
AC_PATH_PROG([SHELL], [sh], :)
AC_PATH_PROG([LN], [ln], :)

AC_PATH_PROG([WGET], [wget], :)
if test "${WGET}" != ":" ; then
  DOWNLOAD=${WGET}
else
  AC_PATH_PROG([FETCH], [fetch], [:])
  DOWNLOAD=${FETCH}
fi
AC_SUBST(DOWNLOAD)

AC_LANG(C)

AC_PATH_PROG([PERL], [perl])
if test "$PERL" = ''; then
	      AC_MSG_ERROR(perl is mandatory)
fi
# Where to install perl modules?
#  * Packagers are encouraged to use the result of "perl -V:vendorlib"
#  * Our default is:
#    - The result of "perl -V:sitelib" for our default prefix (/usr)
#    - The "$datadir/perl5" directory otherwise (e.g: during "make distcheck")
#
AC_MSG_CHECKING(for perl libdir)
m4_define([_PERL_LIBDIR_DEFAULT], [${datadir}/perl5])dnl
AC_ARG_WITH(perllib,
	[AS_HELP_STRING([--with-perllib=PATH],
		[Location of perl modules @<:@default=]_PERL_LIBDIR_DEFAULT[, or perl -V:sitelib@:>@])],
	[perllibdir=$withval],
	[
		if test "$prefix" = 'NONE' -o "$prefix" = '/usr'; then
			eval `"$PERL" -V:sitelib`
			perllibdir="$sitelib"
		else
			perllibdir='_PERL_LIBDIR_DEFAULT'
		fi
	]
)
AC_SUBST([perllibdir], "$perllibdir")
AC_MSG_RESULT([$perllibdir])
AM_CONDITIONAL([PERL], [test "$perllibdir" != ''])

AC_ARG_ENABLE(dev-mode,
	[  --enable-dev-mode    Turn on developer mode],
	[case "${enableval}" in
	      y|ye|yes) DAHDI_DEVMODE=yes ;;
	      n|no)  DAHDI_DEVMODE=no ;;
	      *) AC_MSG_ERROR(bad value ${enableval} for --enable-dev-mode)  ;;
	esac])
AC_SUBST(DAHDI_DEVMODE)
AM_CONDITIONAL([DAHDI_DEVMODE], [test "$DAHDI_DEVMODE" = 'yes'])

AC_MSG_CHECKING(for -Wdeclaration-after-statement support)
if $(${CC} -Wdeclaration-after-statement -S -o /dev/null -xc /dev/null > /dev/null 2>&1); then
	AC_MSG_RESULT(yes)
	DAHDI_DECLARATION_AFTER_STATEMENT=-Wdeclaration-after-statement
else
	AC_MSG_RESULT(no)
	DAHDI_DECLARATION_AFTER_STATEMENT=
fi
AC_SUBST(DAHDI_DECLARATION_AFTER_STATEMENT)

# Checks for header files.
AC_CHECK_HEADERS([sys/soundcard.h linux/soundcard.h])
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netinet/in.h stdint.h stdlib.h string.h sys/ioctl.h sys/param.h sys/socket.h sys/time.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for libraries.
AC_CHECK_LIB([m], [cos])
AC_CHECK_LIB([pthread], [pthread_create])

AST_EXT_LIB_SETUP([DAHDI], [DAHDI], [dahdi])
AST_EXT_LIB_SETUP([NEWT], [newt], [newt])
AST_EXT_LIB_SETUP([USB], [usb], [usb])
AST_EXT_LIB_SETUP([PCAP], [pcap], [pcap])

AST_C_DEFINE_CHECK([DAHDI], [DAHDI_CODE], [dahdi/user.h])
DAHDI23_DIR="${DAHDI_DIR}"
AST_C_DEFINE_CHECK([DAHDI23], [DAHDI_CONFIG_NTTE], [dahdi/user.h])
AST_EXT_LIB_CHECK([NEWT], [newt], [newtBell], [newt.h])
AST_EXT_LIB_CHECK([USB], [usb], [usb_init], [usb.h])
AST_EXT_LIB_CHECK([PCAP], [pcap], [pcap_compile], [pcap.h])

saved_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $DAHDI_INCLUDE"
AC_CHECK_DECL([DAHDI_TXMIRROR], [DAHDI_TXMIRROR=1], [DAHDI_TXMIRROR=0], [#include <dahdi/user.h>])
CPPFLAGS="$saved_CPPFLAGS"

AM_CONDITIONAL([PBX_NEWT], [test "$PBX_NEWT" = "1"])
AM_CONDITIONAL([PBX_PCAP], [test "$PBX_PCAP" = "1" -a "$DAHDI_TXMIRROR" = "1"])

AC_CHECK_FUNCS([semtimedop])
AC_CHECK_FUNCS([alarm bzero gettimeofday memset pow regcomp select socket strcasecmp strchr strdup strerror strrchr strstr strtol strtoul])
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC

# What the user asked for? libusb/libusbx
AC_ARG_WITH([libusbx],
	[AS_HELP_STRING([--without-libusbx], [Disable libusbx @<:@default=check@:>@])],
	[],
	[with_libusbx=check])
AC_ARG_WITH([libusb],
	[AS_HELP_STRING([--without-libusb], [Disable libusb-0.1 @<:@default=check@:>@])],
	[],
	[with_libusb=check])

# LIBUSBX tests
have_libusbx=
AS_IF([test "$with_libusbx" != "no"],
	[PKG_CHECK_MODULES([LIBUSBX], [libusb-1.0],
		[AC_CHECK_LIB([usb-1.0], [libusb_error_name],
			[
				have_libusbx="yes"
				AC_DEFINE([HAVE_LIBUSBX], [1], [Define to 1 if you have <libusb.h>.])
				AC_SUBST([PBX_USB], [1])
			], [
				if test "$with_libusbx" != "yes"; then
					AC_MSG_NOTICE([Ignoring old libusb-1.0, trying fallback to old libusb])
				fi
			])
		], [:])
	], [])

AS_IF([test "$with_libusbx" = "yes" -a "$have_libusbx" = ""],
	[AC_MSG_ERROR([No libusbx found (--with-libusbx was given)])])

# LIBUSB tests
have_libusb=
AS_IF([test "$with_libusb" != "no"],
	[AS_IF([test "$have_libusbx" != ""],
		[
			AS_IF([test "$with_libusbx" = "check"], [
				AC_MSG_NOTICE([Skipping LIBUSB tests (have good LIBUSBX)])
				])
		], [
			PKG_CHECK_MODULES([LIBUSB], [libusb],
				[
					have_libusb="yes"
					AC_SUBST([PBX_USB], [1])
				]
				[AC_DEFINE([HAVE_LIBUSB], [1], [Define to 1 if you have <usb.h>.])],
				[:]
				)
		])
	], [])

AS_IF([test "$with_libusb" = "yes" -a "$have_libusb" = ""],
	[AC_MSG_ERROR([No libusb found (--with-libusb was given)])])

AM_CONDITIONAL([LIBUSBX], [test "$have_libusbx" != ""])
AM_CONDITIONAL([LIBUSB], [test "$have_libusb" != ""])
AM_CONDITIONAL([PBX_USB], [test "$PBX_USB" = "1"])


PBX_HDLC=0
AC_MSG_CHECKING([for GENERIC_HDLC_VERSION version 4 in linux/hdlc.h])
AC_COMPILE_IFELSE(
    [ AC_LANG_PROGRAM( [#include <linux/hdlc.h>],
	    [#if defined(GENERIC_HDLC_VERSION) && GENERIC_HDLC_VERSION >= 4
		int foo = 0;
		#else
		int foo = bar;
		#endif
		0])],
	[AC_MSG_RESULT(yes)
	 PBX_HDLC=1],
	[AC_MSG_RESULT(no)]
)
if test $PBX_HDLC = 0; then
    AC_MSG_CHECKING([for GENERIC_HDLC_VERSION version 4 in linux/hdlc/ioctl.h])
    AC_COMPILE_IFELSE(
	    [ AC_LANG_PROGRAM( [
		    #include <sys/socket.h>
			#include <linux/if.h>],
			[#if defined(GENERIC_HDLC_VERSION) && GENERIC_HDLC_VERSION >= 4
			int foo = 0;
			#else
			int foo = bar;
			#endif
			0])],
		[AC_MSG_RESULT(yes)
		 PBX_HDLC=1],
		[AC_MSG_RESULT(no)]
	)
fi

if test "x${PBX_HDLC}" != "x1"; then
   AC_MSG_NOTICE([GENERIC_HDLC_VERSION (version 4) not found, disabling sethdlc.])
fi

AC_SUBST(PBX_HDLC)
AM_CONDITIONAL([PBX_HDLC], [test "$PBX_HDLC" = "1"])

AC_ARG_WITH(selinux,
	[AS_HELP_STRING([--with-selinux],
			[enable (with) / disable (without) SELinux])],
	[USE_SELINUX=$withval],
	[ if test ! -x /usr/sbin/sestatus; then 
	   	USE_SELINUX=no;
	  elif /usr/sbin/sestatus | grep "SELinux status:" | grep -q "enabled"; then
		USE_SELINUX=yes
	  fi
	]
)


AC_SUBST(USE_SELINUX)

# for asciidoc before ver. 7, the backend must be stated explicitly:
ASCIIDOC='asciidoc'
asciidoc_ver=`asciidoc --version 2>&1 | awk '/^asciidoc /{print $2}' | cut -d. -f 1 | head -n 1`
if test "$asciidoc_ver" != '' && test $asciidoc_ver -lt 7; then
	ASCIIDOC="asciidoc -b xhtml"
fi
AC_SUBST(ASCIIDOC)

AC_ARG_WITH(ppp,
	    [AS_HELP_STRING([--with-ppp=PATH],[Use PPP support from PATH])],
	    [],
	    [with_ppp=check]
	    )
# somebody will fix that
default_ppp_path=/usr

case "$with_ppp" in
	yes|check) ppp_path="$default_ppp_path";;
	no) ppp_path='' ;;
	*) ppp_path="$with_ppp" ;;
esac

level_file="$ppp_path/include/pppd/patchlevel.h"
PPP_VERSION=
if test "$ppp_path" != '' && test -r "$level_file"; then
	PPPD_VERSION=`awk -F '"' '/VERSION/ { print $$2; }' $level_file`
fi

case "$with_ppp" in 
	check|no) :;;
	*) 
	# If we asked explicitly for ppp support
	if test "$PPPD_VERSION" = ''; then
		# but have not detected it
		AC_MSG_ERROR(failed to find pppd/patchlevel.h: no ppp support.)
	fi
	;;
esac

AC_SUBST(PPPD_VERSION)
AM_CONDITIONAL([PPPD], [test "$PPPD_VERSION" != ''])

if test "x${PBX_DAHDI}" != "x1"; then
   AC_MSG_NOTICE([***])
   AC_MSG_NOTICE([*** Building this package requires DAHDI support. *** ])
   AC_MSG_NOTICE([*** Please install the dahdi-linux package. ***])
   AC_MSG_NOTICE([***])
   exit 1
fi

if test "x${PBX_DAHDI23}" != "x1"; then
   AC_MSG_NOTICE([***])
   AC_MSG_NOTICE([*** Building this package requires DAHDI support (>= 2.3) *** ])
   AC_MSG_NOTICE([*** Please install a recent dahdi-linux package. ***])
   AC_MSG_NOTICE([***])
   exit 1
fi

AC_SUBST(PPPD_VERSION)

# Taken from octasic-helper
AC_SUBST([USE_OCTASIC], [yes])
AM_CONDITIONAL([USE_OCTASIC], [test "$USE_OCTASIC" = 'yes'])
octasic_dir='$(top_srcdir)/xpp/oct612x'
AC_SUBST([OCTASIC_CFLAGS], ["-I$octasic_dir/include -I$octasic_dir/octdeviceapi -I$octasic_dir/octdeviceapi/oct6100api"])

AC_SUBST([TOOLSVERSION], m4_esyscmd([build_tools/make_version . dahdi/tools]))

AC_CONFIG_FILES([
	makeopts
	version.c
	Makefile
	doc/Makefile
	hotplug/Makefile
	ppp/Makefile
	xpp/Makefile
	xpp/xtalk/Makefile
	xpp/oct612x/Makefile
	xpp/perl_modules/Makefile
	])
AC_OUTPUT

AC_MSG_NOTICE(*** dahdi-tools build successfully configured ***)
