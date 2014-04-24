#
# libtsm - Global Makefile
# Copyright (c) 2012-2013 David Herrmann <dh.herrmann@gmail.com>
#

#
# Library Version Numbers
#

LIBTSM_CURRENT = 3
LIBTSM_REVISION = 0
LIBTSM_AGE = 0

#
# Global Configurations and Initializations
#

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory
AUTOMAKE_OPTIONS = color-tests
AM_DISTCHECK_CONFIGURE_FLAGS = \
	--enable-debug \
	--enable-optimizations

SUBDIRS = .

.DELETE_ON_ERROR:

include_HEADERS =
EXTRA_DIST = \
	README \
	COPYING \
	LICENSE_htable \
	NEWS
CLEANFILES =
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =
TPHONY =

TESTS =
MEMTESTS =
check_PROGRAMS =
bin_PROGRAMS =
lib_LTLIBRARIES =
noinst_LTLIBRARIES =

#
# Default CFlags
# Make all files include "config.h" by default. This shouldn't cause any
# problems and we cannot forget to include it anymore.
#
# Also make the linker discard all unused symbols.
#
# When compiling in debug mode, we enable debug symbols so debugging with gdb
# is easier. If optimizations are disabled, we pass -O0 to the compiler.
# Otherwise, we use standard optimizations -O2.
#

AM_CFLAGS = \
	-Wall \
	-pipe \
	-fno-common \
	-ffast-math \
	-fdiagnostics-show-option \
	-fno-strict-aliasing \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections
AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I $(srcdir)/src \
	-I $(srcdir)/src/shared
AM_LDFLAGS =

if BUILD_HAVE_GNU_LD
AM_LDFLAGS += \
	-Wl,--as-needed \
	-Wl,--gc-sections \
	-Wl,-z,relro \
	-Wl,-z,now
else
if BUILD_IS_DARWIN
AM_LDFLAGS += \
	-Wl,-dead_strip \
	-Wl,-dead_strip_dylibs \
	-Wl,-bind_at_load
endif
endif

if BUILD_ENABLE_DEBUG
AM_CFLAGS += -g
endif

if BUILD_ENABLE_OPTIMIZATIONS
AM_CFLAGS += -O2
else
AM_CFLAGS += -O0
endif

#
# SHL - Static Helper Library
# The SHL subsystem contains several small code pieces used all over libtsm and
# other applications.
#

noinst_LTLIBRARIES += libshl.la

libshl_la_SOURCES = \
	src/shared/shl-array.h \
	src/shared/shl-htable.h \
	src/shared/shl-htable.c \
	src/shared/shl-llog.h \
	src/shared/shl-macro.h \
	src/shared/shl-ring.h \
	src/shared/shl-ring.c
libshl_la_CPPFLAGS = $(AM_CPPFLAGS)
libshl_la_LDFLAGS = $(AM_LDFLAGS)
libshl_la_LIBADD = $(AM_LIBADD)

#
# libtsm
# Main library build instructions
#

lib_LTLIBRARIES += libtsm.la
noinst_LTLIBRARIES += libtsm-test.la
include_HEADERS += src/tsm/libtsm.h
pkgconfig_DATA += src/tsm/libtsm.pc
EXTRA_DIST += \
	src/tsm/libtsm.pc.in \
	src/tsm/libtsm.sym

libtsm_la_SOURCES = \
	src/tsm/libtsm.h \
	src/tsm/libtsm-int.h \
	src/tsm/tsm-render.c \
	src/tsm/tsm-screen.c \
	src/tsm/tsm-selection.c \
	src/tsm/tsm-unicode.c \
	src/tsm/tsm-vte.c \
	src/tsm/tsm-vte-charsets.c \
	external/wcwidth.h \
	external/wcwidth.c \
	external/xkbcommon-keysyms.h
libtsm_test_la_SOURCES = $(libtsm_la_SOURCES)

libtsm_la_CPPFLAGS = $(AM_CPPFLAGS)
libtsm_test_la_CPPFLAGS = $(AM_CPPFLAGS)

libtsm_la_LIBADD = libshl.la
libtsm_test_la_LIBADD = libshl.la

EXTRA_libtsm_la_DEPENDENCIES = $(top_srcdir)/src/tsm/libtsm.sym

libtsm_la_LDFLAGS = \
	$(AM_LDFLAGS) \
	-version-info $(LIBTSM_CURRENT):$(LIBTSM_REVISION):$(LIBTSM_AGE)

if BUILD_HAVE_GNU_LD
libtsm_la_LDFLAGS += \
	-Wl,--version-script="$(top_srcdir)/src/tsm/libtsm.sym"
endif

libtsm_test_la_LDFLAGS = \
	$(AM_LDFLAGS)

if BUILD_HAVE_XKBCOMMON
libtsm_la_CPPFLAGS += $(XKBCOMMON_CFLAGS)
libtsm_test_la_CPPFLAGS += $(XKBCOMMON_CFLAGS)
endif

#
# GtkTsm - Example
#

if BUILD_ENABLE_GTKTSM
bin_PROGRAMS += gtktsm
endif

gtktsm_SOURCES = \
	src/gtktsm/gtktsm.c \
	src/gtktsm/gtktsm-app.h \
	src/gtktsm/gtktsm-app.c \
	src/gtktsm/gtktsm-terminal.h \
	src/gtktsm/gtktsm-terminal.c \
	src/gtktsm/gtktsm-win.h \
	src/gtktsm/gtktsm-win.c \
	src/shared/shl-pty.h \
	src/shared/shl-pty.c
gtktsm_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(GTKTSM_CFLAGS)
gtktsm_LDADD = \
	-lm \
	libshl.la \
	libtsm.la \
	$(GTKTSM_LIBS)
gtktsm_LDFLAGS = \
	$(AM_LDFLAGS)

#
# Tests
# We add a separate "memcheck" target which runs valgrind on all tests in
# MEMTESTS. Note that we fail if _any_ leak is detected by valgrind. Thus, you
# need to have valgrind installed and libcheck running properly (without leaks)
# to make memcheck succeed.
# A separate memcheck-verify actually runs a faulty test and verifies the
# valgrind tests work properly.
#

if BUILD_HAVE_CHECK
check_PROGRAMS += \
	test_htable \
	test_symbol \
	test_valgrind
TESTS += \
	test_htable \
	test_symbol \
	test_valgrind
MEMTESTS += \
	test_htable \
	test_symbol
endif

test_sources = \
	test/test_common.h
test_libs = \
	libshl.la \
	libtsm-test.la \
	$(CHECK_LIBS)
test_cflags = \
	$(AM_CPPFLAGS) \
	$(CHECK_CFLAGS)
test_lflags = \
	$(AM_LDFLAGS)

test_htable_SOURCES = test/test_htable.c $(test_sources)
test_htable_CPPFLAGS = $(test_cflags)
test_htable_LDADD = $(test_libs)
test_htable_LDFLAGS = $(test_lflags)

test_symbol_SOURCES = test/test_symbol.c $(test_sources)
test_symbol_CPPFLAGS = $(test_cflags)
test_symbol_LDADD = $(test_libs)
test_symbol_LDFLAGS = $(test_lflags)

test_valgrind_SOURCES = test/test_valgrind.c $(test_sources)
test_valgrind_CPPFLAGS = $(test_cflags)
test_valgrind_LDADD = $(test_libs)
test_valgrind_LDFLAGS = $(test_lflags)

EXTRA_DIST += test.supp

VALGRIND = CK_FORK=no valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --leak-resolution=high --error-exitcode=1 --suppressions=$(top_builddir)/test.supp

# verify that test_valgrind actually leaks data
memcheck-verify: check
	$(AM_V_GEN)$(VALGRIND) --log-file=/dev/null ./test_valgrind >/dev/null ; test 1 = $$?

TPHONY += memcheck-verify

# run memcheck tests via valgrind
memcheck: memcheck-verify
	$(AM_V_GEN)for i in $(MEMTESTS) ; do \
		$(VALGRIND) --log-file=$(top_builddir)/$$i.memlog \
			$(top_builddir)/$$i >/dev/null || (echo "memcheck failed on: $$i" ; exit 1) ; \
	done

TPHONY += memcheck memcheck-verify

distcheck-hook: memcheck

#
# Phony targets
#

.PHONY: $(TPHONY)

#
# Empty .SECONDARY target causes alle intermediate files to be treated as
# secondary files. That is, they don't get deleted after make finished.
#

.SECONDARY:
