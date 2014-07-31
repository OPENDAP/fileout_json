# Tests

# AUTOMAKE_OPTIONS = foreign subdir-objects

AM_CPPFLAGS = -I$(top_srcdir) $(BES_CPPFLAGS)
AM_LDADD = $(BES_DAP_LIBS) 
if CPPUNIT
AM_CPPFLAGS += $(CPPUNIT_CFLAGS)
AM_LDADD += $(CPPUNIT_LIBS)
endif

# These are not used by automake but are often useful for certain types of
# debugging. Set CXXFLAGS to this in the nightly build using export ...
CXXFLAGS_DEBUG = -g3 -O0  -Wall -W -Wcast-align -Werror
TEST_COV_FLAGS = -ftest-coverage -fprofile-arcs

DISTCLEANFILES = test_config.h

CLEANFILES = *.dbg *.log

EXTRA_DIST = baselines test_config.h.in

check_PROGRAMS = $(UNIT_TESTS)

TESTS = $(UNIT_TESTS)

############################################################################
# Unit Tests
#

UNIT_TESTS = FoJsonTest

SRCS = ../FoJsonTransform.cc ../FoW10nJsonTransform.cc ../utils.cc

FoJsonTest_SOURCES = FoJsonTest.cc $(SRCS)
FoJsonTest_LDADD = $(AM_LDADD)

noinst_HEADERS = test_config.h
