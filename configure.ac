#
# libtsm - build configuration script
# Copyright (c) 2012-2013 David Herrmann <dh.herrmann@gmail.com>
#

AC_PREREQ(2.68)

AC_INIT([libtsm],
        [1],
        [https://github.com/dvdhrm/libtsm/issues],
        [libtsm],
        [http://dvdhrm.github.io/libtsm])
AC_CONFIG_SRCDIR([src/tsm_vte.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER(config.h)
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CANONICAL_HOST

AM_INIT_AUTOMAKE([foreign 1.11 subdir-objects dist-xz no-dist-gzip tar-pax -Wall -Werror -Wno-portability])
AM_SILENT_RULES([yes])

#
# Don't add a default "-g -O2" if CFLAGS wasn't specified. For debugging it is
# often more convenient to have "-g -O0". You can still override it by
# explicitly setting it on the command line.
#

: ${CFLAGS=""}

AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_SED
AC_PROG_MKDIR_P
AC_PROG_LN_S
AC_PROG_GREP
AC_PROG_AWK

LT_PREREQ(2.2)
LT_INIT

#
# pkg-config dependencies
# This unconditionally checks for all dependencies even if they are disabled. We
# later look whether all required depedencies are met and finish the
# configuration. We group similar packages into one logical group here to avoid
# having variables for each single library.
# This, however, makes ./configure output very unintuitive error messages if a
# package is not found so we must make sure we print more verbose messages
# ourself.
#

PKG_CHECK_MODULES([XKBCOMMON], [xkbcommon],
                  [have_xkbcommon=yes], [have_xkbcommon=no])
AC_SUBST(XKBCOMMON_CFLAGS)
if test "x$have_xkbcommon" = "xyes" ; then
        AC_DEFINE([BUILD_HAVE_XKBCOMMON], [1], [Have xkbcommon library])
fi
AM_CONDITIONAL([BUILD_HAVE_XKBCOMMON], [test "x$have_xkbcommon" = "xyes"])

#
# Parse arguments
# This parses all arguments that are given via "--enable-XY" or "--with-XY" and
# saves the results in local variables.
#

# debug
AC_MSG_CHECKING([whether to build with debugging on])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [whether to build with debugging on])])
if test "x$enable_debug" = "x" ; then
        enable_debug="yes (default)"
fi
AC_MSG_RESULT([$enable_debug])
if test "x${enable_debug% *}" = "xyes" ; then
        enable_debug="yes"
        AC_DEFINE([BUILD_ENABLE_DEBUG], [1], [Enable debug mode])
else
        enable_debug="no"
        AC_DEFINE([NDEBUG], [1], [No Debug])
fi
AM_CONDITIONAL([BUILD_ENABLE_DEBUG],
               [test "x$enable_debug" = "xyes"])

# optimizations
AC_MSG_CHECKING([whether to enable code optimizations])
AC_ARG_ENABLE([optimizations],
              [AS_HELP_STRING([--disable-optimizations],
                              [whether to disable code optimizations])])
if test "x$enable_optimizations" = "x" ; then
        enable_optimizations="yes (default)"
fi
AC_MSG_RESULT([$enable_optimizations])
if test "x${enable_optimizations% *}" = "xyes" ; then
        enable_optimizations="yes"
else
        enable_optimizations="no"
fi
AM_CONDITIONAL([BUILD_ENABLE_OPTIMIZATIONS],
               [test "x$enable_optimizations" = "xyes"])

#
# Makefile vars
# After everything is configured, we create all makefiles.
#

AC_CONFIG_FILES([Makefile
                 docs/libtsm.pc])
AC_OUTPUT

#
# Configuration output
# Show configuration to the user so they can check whether everything was
# configured as expected.
#

AC_MSG_NOTICE([Build configuration:

               prefix: $prefix
          exec-prefix: $exec_prefix
               libdir: $libdir
           includedir: $includedir

  Miscellaneous Options:
                debug: $enable_debug
        optimizations: $enable_optimizations

        Run "${MAKE-make}" to start compilation process])
