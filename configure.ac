#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([b0rk], [0.1], [ian@geekprojects.com])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_HEADERS([include/config.h])

# Checks for programs.
AM_PROG_AR
AC_PROG_AWK
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([float.h stdint.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT64_T
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memset strdup])

PKG_CHECK_MODULES(libgeek, libgeek >= 1)
CPPFLAGS="$CPPFLAGS $libgeek_CFLAGS"
LIBS="$LIBS $libgeek_LIBS"

CPPFLAGS="$CPPFLAGS -Wall -Werror -ggdb"

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/libb0rk/Makefile
    src/libb0rk/packages/Makefile
    src/libb0rk/packages/system/io/Makefile
    src/libb0rk/packages/system/lang/Makefile
    src/libb0rk/packages/system/Makefile
    src/b0rk/Makefile
    include/Makefile
    include/b0rk/Makefile
    b0rk.pc
])
AC_OUTPUT
