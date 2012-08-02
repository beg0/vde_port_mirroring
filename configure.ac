#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([vde_port_mirroring], [1.0], [beg0@free.fr])
AC_CONFIG_SRCDIR([span_port.c])
AC_CONFIG_FILES([vde2_config.mak])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC

# Check for VDE sources
AC_ARG_WITH([vde2-include-path],
  [AS_HELP_STRING([--with-vde2-include-path], [location of VDE2 header files])],
  [VDE2_INCLUDE="$withval"],
  [VDE2_INCLUDE="/usr/local/src/vde2"])
AC_SUBST([VDE2_INCLUDE])

function test_with_include {
    local hdr=$1;
    local include_path=$2;
    local CPPFLAGS=-I$include_path;
    AC_CHECK_HEADER($hdr,[],AC_MSG_ERROR([cannot locate $hdr in $include_path]))
}

test_with_include vde.h $VDE2_INCLUDE;
AC_OUTPUT
