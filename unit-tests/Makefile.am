# Tests

AUTOMAKE_OPTIONS = foreign

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

CLEANFILES = *.dbg *.log *.sum site.exp *.nc

EXTRA_DIST = data baselines test_config.h.in bes.conf.in fonc_handlerTest.at $(TESTSUITE) atlocal.in $(srcdir)/package.m4

check_PROGRAMS = $(UNIT_TESTS)

############################################################################
# Unit Tests
#


UNIT_TESTS = FoJsonTest

SRCS = ../FoJsonTransform.cc


FoJsonTest_SOURCES = FoJsonTest.cc $(SRCS)
FoJsonTest_LDADD = $(AM_LDADD)



noinst_HEADERS = test_config.h


############## Autotest follows #####################

AUTOM4TE = autom4te
DIRS_EXTRA = bes-testsuite common_tests.exp

TESTSUITE = $(srcdir)/fojson_handlerTest

status:
	echo "AM_LDADD: " $(AM_LDADD)

check-local: $(UNIT_TESTS) atconfig atlocal $(TESTSUITE)
	export PATH="@NC_BIN_PATH@:$$PATH"; $(TESTSUITE) $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

distclean-local:
	-rm atconfig

AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(srcdir)/fojson_handlerTest.at $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@

# The `:;' works around a Bash 3.2 bug when the output is not writeable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [@PACKAGE_NAME@])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [@PACKAGE_TARNAME@])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [@PACKAGE_VERSION@])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [@PACKAGE_STRING@])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [@PACKAGE_BUGREPORT@])'; \
	} >'$(srcdir)/package.m4'

