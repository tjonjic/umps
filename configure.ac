m4_define([umps_major_version], [1])
m4_define([umps_minor_version], [9])
m4_define([umps_micro_version], [7])

m4_define([umps_version], [umps_major_version.umps_minor_version.umps_micro_version])

AC_PREREQ(2.64)
AC_INIT([umps], [umps_version], [jonjic@cs.unibo.it])
AC_CONFIG_AUX_DIR([build-aux])

AC_CANONICAL_SYSTEM
AC_C_BIGENDIAN
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AC_CONFIG_SRCDIR([src/umps/device.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.

AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_AS
AM_PROG_LD
AC_PROG_RANLIB

# Initialize libtool
LT_INIT

# Check for cross toolchain

AC_ARG_WITH([mips-tool-prefix],
            [AS_HELP_STRING([--with-mips-tool-prefix=PREFIX],
            [specify prefix for the cross toochain tools])],
            [xt_prefixes=$withval])

if test -z "$xt_prefixes" ; then
    if test "x$ac_cv_c_bigendian" = xyes ; then
        xt_prefixes="mips-elf- mips-linux-"
    else
        xt_prefixes="mipsel-elf- mipsel-linux-"
    fi
fi

for pref in $xt_prefixes ; do
    AC_PATH_PROG([XCGCC], [${pref}gcc], [no])
    if test "x${XCGCC}" != xno ; then
        xt_prefix=$pref
        break
    fi
done

if test "x${XCGCC}" = xno ; then
    AC_MSG_ERROR([MIPS toolchain (gcc) not found.])
fi

AC_PATH_PROG([XCAS], [${xt_prefix}as], [no])
if test "x${XCAS}" = xno ; then
    AC_MSG_ERROR([MIPS toolchain (as) not found.])
fi

AC_PATH_PROG([XCLD], [${xt_prefix}ld], [no])
if test "x${XCLD}" = xno ; then
    AC_MSG_ERROR([MIPS toolchain (ld) not found.])
fi


AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I m4")


# Do you have external libelf?
AC_ARG_WITH([elf],[  --with-elf[=PATH]         libelf-dev patname if not in standard location.], [ELFLIB="${withval}"], [ELFLIB="no"])

ELFLIB=`readlink -f $ELFLIB`

# If we are using external libs we need some fixing before checks.

if test "x$ELFLIB" != "xno"; then
	UMPSCPPFLAG="${UMPSCPPFLAG} -I${ELFLIB}"
	LIBS="${LIBS} -L${ELFLIB}"
	AC_DEFINE([__LIBELF_INTERNAL_], [0], [You are using external libelf.])
fi

# Checks for libraries.

AC_CHECK_LIB([elf], [elf_version], [ELF_LIBS=-lelf],
	     AC_MSG_ERROR([*** Libelf not found. Specify a different path or install it. --help for info.])
	     )
AC_SUBST([ELF_LIBS])

AC_CHECK_LIB([dl], [dlopen], [DL_LIBS=-ldl], [AC_MSG_ERROR([*** Libdl not found.])])
AC_SUBST([DL_LIBS])

PKG_CHECK_MODULES([SIGCPP], sigc++-2.0)
AC_SUBST(SIGCCP_CFLAGS)

BOOST_REQUIRE([1.34])

AT_WITH_QT([], [+no_keywords])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h libelf.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([getcwd memset socket strchr strerror strstr strtoul])

# Setting the arch-dependent prefix and flags.
if test "x$ac_cv_c_bigendian" = "xyes" ; then
    LDCOREFLAGS="-T ../support/elf32btsmip.h.umpscore.x"
    LDAOUTFLAGS="-T ../support/elf32btsmip.h.umpsaout.x"
    LDCORESCRIPT="elf32btsmip.h.umpscore.x"
    LDAOUTSCRIPT="elf32btsmip.h.umpsaout.x"
else
    LDCOREFLAGS="-T ../support/elf32ltsmip.h.umpscore.x"
    LDAOUTFLAGS="-T ../support/elf32ltsmip.h.umpsaout.x"
    LDCORESCRIPT="elf32ltsmip.h.umpscore.x"
    LDAOUTSCRIPT="elf32ltsmip.h.umpsaout.x"
fi
AC_SUBST(LDCORESCRIPT)
AC_SUBST(LDAOUTSCRIPT)
AC_SUBST(LDCOREFLAGS)
AC_SUBST(LDAOUTFLAGS)

# Checking for hosttype.
case "${host}" in
	*-*-linux*) 
	AC_DEFINE([LINUX], [linux], [Linuxonly convention])
	;;
esac

AC_SUBST(UMPSCPPFLAG)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/include/Makefile
		 src/base/Makefile
		 src/umps/Makefile
		 src/support/Makefile
		 src/support/bios/Makefile
                 src/frontends/Makefile
                 src/frontends/qmps/Makefile
                 src/tests/Makefile])

AC_OUTPUT
