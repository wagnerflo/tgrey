AM_CXXFLAGS = -Wall -Werror

# build a static library with all commonly used functions and classes;
# this allows easy linking of multiple output binaries as well as the
# unittests
#
noinst_LIBRARIES = libtgrey.a
libtgrey_a_SOURCES = src/policy.cc src/database.cc \
                     src/misc.cc src/logging.cc
libtgrey_a_CPPFLAGS = $(libtdb_CFLAGS)

# the actual output binaries to be installed by the package
#
libexec_PROGRAMS = tgreylist
#sbin_PROGRAMS = tgreyclean

tgreylist_SOURCES = src/tgreylist.cc
tgreylist_CPPFLAGS = $(libtdb_CFLAGS) -DCONFIG_TGREY_DB=\"$(TGREY_DB)\"
tgreylist_LDADD = $(libtdb_LIBS) libtgrey.a

#tgreyclean_SOURCES = src/tgreyclean.cc
#tgreyclean_CPPFLAGS = $(libtdb_CFLAGS) -DCONFIG_TGREY_DB=\"$(TGREY_DB)\"
#tgreyclean_LDADD = $(libtdb_LIBS) libtgrey.a

# man pages to install
#
#dist_man_MANS = man/tgrey.5 man/tgreylist.8 man/tgreyclean.1

# also build a set of utilities for running the tests; these are confined
# to the tests subdirectory
#
check_PROGRAMS = tests/mktriplet
tests_mktriplet_SOURCES = tests/mktriplet.cc
tests_mktriplet_CPPFLAGS = -Isrc
tests_mktriplet_LDADD = libtgrey.a

# define the unit and system tests to run
#
TESTS = tests/by-addrv4,triplet.triplet tests/by-name,triplet.triplet
TEST_SUITE_LOG = tests/suite.log

TEST_EXTENSIONS = .triplet
TRIPLET_LOG_COMPILER = tests/mktriplet.check
