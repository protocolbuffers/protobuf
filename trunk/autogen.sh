#!/bin/sh

# Run this script to generate the configure script and other files that will
# be included in the distribution.  These files are not checked in because they
# are automatically generated.

set -e

# Check that we're being run from the right directory.
if test ! -f src/google/protobuf/stubs/common.h; then
  cat >&2 << __EOF__
Could not find source code.  Make sure you are running this script from the
root of the distribution tree.
__EOF__
  exit 1
fi

# Check that gtest is present.  Usually it is already there since the
# directory is set up as an SVN external.
if test ! -e gtest; then
  echo "Google Test not present.  Fetching gtest-1.5.0 from the web..."
  curl http://googletest.googlecode.com/files/gtest-1.5.0.tar.bz2 | tar jx
  mv gtest-1.5.0 gtest
fi

set -ex

# Temporary hack:  Must change C runtime library to "multi-threaded DLL",
#   otherwise it will be set to "multi-threaded static" when MSVC upgrades
#   the project file to MSVC 2005/2008.  vladl of Google Test says gtest will
#   probably change their default to match, then this will be unnecessary.
#   One of these mappings converts the debug configuration and the other
#   converts the release configuration.  I don't know which is which.
sed -i -e 's/RuntimeLibrary="5"/RuntimeLibrary="3"/g;
           s/RuntimeLibrary="4"/RuntimeLibrary="2"/g;' gtest/msvc/*.vcproj

# TODO(kenton):  Remove the ",no-obsolete" part and fix the resulting warnings.
autoreconf -f -i -Wall,no-obsolete

rm -rf autom4te.cache config.h.in~
exit 0
