#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([monolife], [0.1], [evan@eklitzke.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_SRCDIR([src/monolife.cc])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_LANG([C++])
AM_INIT_AUTOMAKE([dist-bzip2 foreign subdir-objects -Wall -Werror])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB([event], [event_base_new])
AC_CHECK_LIB([monome], [monome_open])

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.

AC_DEFINE([MONOLIFE_DEFAULT_DEVICE], ["/dev/ttyUSB0"], [The default device to use.])
AS_COMPILER_FLAG([-Wall], [AX_APPEND_FLAG([-Wall])])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
