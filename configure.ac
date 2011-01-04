m4_define([umps_major_version], [1])
m4_define([umps_minor_version], [9])
m4_define([umps_micro_version], [2])

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

AC_PREFIX_DEFAULT(/usr/local)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_LIBTOOL
AM_PROG_AS
AM_PROG_LD

# Initialize libtool
LT_INIT

AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I m4")

PKG_CHECK_MODULES([SIGCPP], sigc++-2.0)
AC_SUBST(SIGCCP_CFLAGS)

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

AC_CHECK_LIB([elf], [elf_version], [],
	     AC_MSG_ERROR([*** Libelf not found. Specify a different path or install it. --help for info.])
	     )

BOOST_REQUIRE([1.34])

AT_WITH_QT([], [+no_keywords])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h libelf.h stdint.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# We can specify crosscompiler PATH or use the conventional /usr/local/bin
AC_ARG_WITH([cross],[  --with-cross[=PATH]       pathname of the crosscompiler set.], [CROSSBIN="${withval}"], [CROSSBIN="/usr/local/bin"])

CROSSBIN=`readlink -f $CROSSBIN`

# Checks for library functions.
AC_FUNC_MEMCMP
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([getcwd memset socket strchr strerror strstr strtoul])

# Setting the arch-dependent prefix and flags.
if test "x$ac_cv_c_bigendian" = "xyes"
then
	crosscompiler_prefix="mips-linux"
	LDCOREFLAGS="-T ../support/elf32btsmip.h.umpscore.x"
	LDAOUTFLAGS="-T ../support/elf32btsmip.h.umpsaout.x"
	LDCORESCRIPT="elf32btsmip.h.umpscore.x"
	LDAOUTSCRIPT="elf32btsmip.h.umpsaout.x"
else
	crosscompiler_prefix="mipsel-linux"
	LDCOREFLAGS="-T ../support/elf32ltsmip.h.umpscore.x"
	LDAOUTFLAGS="-T ../support/elf32ltsmip.h.umpsaout.x"
	LDCORESCRIPT="elf32ltsmip.h.umpscore.x"
	LDAOUTSCRIPT="elf32ltsmip.h.umpsaout.x"
fi

# Checking for gcc, ar, as and ld crosscompiling tools.
AC_CHECK_PROGS([GCCUMPS], ["${crosscompiler_prefix}"-gcc], ["no"], ["${CROSSBIN}"])
if test "x${GCCUMPS}" = "xno"
then
	AC_MSG_ERROR([*** ${crosscompiler_prefix}-gcc not found in ${CROSSBIN}. Specify a different path or install it.])
fi
AC_CHECK_PROGS([ASUMPS], ["${crosscompiler_prefix}"-as], ["no"], ["${CROSSBIN}"])
if test "x${ASUMPS}" = "xno"
then
	AC_MSG_ERROR([*** ${crosscompiler_prefix}-as not found in ${CROSSBIN}. Specify a different path or install it.])
fi
AC_CHECK_PROGS([ARUMPS], ["${crosscompiler_prefix}"-ar], ["no"], ["${CROSSBIN}"])
if test "x${ARUMPS}" = "xno"
then
	AC_MSG_ERROR([*** ${crosscompiler_prefix}-ar not found in ${CROSSBIN}. Specify a different path or install it.])
fi
AC_CHECK_PROGS([LDUMPS], ["${crosscompiler_prefix}"-ld], ["no"], ["${CROSSBIN}"])
if test "x${LDUMPS}" = "xno"
then
	AC_MSG_ERROR([*** ${crosscompiler_prefix}-ld not found in ${CROSSBIN}. Specify a different path or install it.])
fi

# If cross-tools path have to be in the form /dir/
if test `expr ${CROSSBIN} : '.*/$'` -eq "0"
then
	CROSSBIN=${CROSSBIN}/
fi

# Checking for hosttype.
case "${host}" in
	*-*-linux*) 
	AC_DEFINE([LINUX], [linux], [Linuxonly convention])
	;;
esac

# Adding cross-tools complete pathname.
GCCUMPS=${CROSSBIN}${GCCUMPS}
ASUMPS=${CROSSBIN}${ASUMPS}
ARUMPS=${CROSSBIN}$ARUMPS}
LDUMPS=${CROSSBIN}${LDUMPS}

# We have to setup example's Makefiles
dnl for i in `find ./*example* -name Makefile.mod`
dnl do
dnl 	rm -rf `dirname $i`/Makefile
dnl 	sed 	-e s%LD_AOUT_FLAGS%${LDAOUTSCRIPT}%g \
dnl 		-e s%LD_CORE_FLAGS%${LDCORESCRIPT}%g \
dnl 		-e s%GCC-UMPS%${GCCUMPS}%g \
dnl 		-e s%LD-UMPS%${LDUMPS}%g \
dnl 		-e s%AS-UMPS%${ASUMPS}%g \
dnl 		-e s%PACKAGENAME%${PACKAGE_NAME}%g \
dnl 		$i >> `dirname $i`/Makefile
dnl done
	
# Setting variable for @@ substitution in Makefile.am
XGCC=${GCCUMPS}
AC_SUBST(XGCC)
CCAS=${ASUMPS}
AC_SUBST(CCAS)
CCASFLAGS="-KPIC"
AC_SUBST(CCASFLAGS)
AC_SUBST(UMPSCPPFLAG)
AC_SUBST(LDCOREFLAGS)
AC_SUBST(LDAOUTFLAGS)
AC_SUBST(LDCORESCRIPT)
AC_SUBST(LDAOUTSCRIPT)

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 src/base/Makefile
		 src/umps/Makefile
		 src/support/Makefile
                 src/frontends/Makefile
                 src/frontends/qmps/Makefile
                 src/tests/Makefile])

AC_OUTPUT
