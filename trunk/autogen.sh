#!/bin/sh

# Run this script to generate the configure script and other files that will
# be included in the distribution.  These files are not checked in because they
# are automatically generated.

# Check that we're being run from the right directory.
if test ! -f src/google/protobuf/stubs/common.h; then
  cat >&2 << __EOF__
Could not find source code.  Make sure you are running this script from the
root of the distribution tree.
__EOF__
  exit 1
fi

if test ! -d gtest; then
  echo "gtest bundle not present.  Downloading gtest-1.3.0 automatically." >&2
  set -ex
  curl http://googletest.googlecode.com/files/gtest-1.3.0.tar.bz2 | tar jx
  mv gtest-1.3.0 gtest

  # Temporary hack:  Must change C runtime library to "multi-threaded DLL",
  #   otherwise it will be set to "multi-threaded static" when MSVC upgrades
  #   the project file to MSVC 2005/2008.  vladl of Google Test says gtest will
  #   probably change their default to match, then this will be unnecessary.
  #   One of these mappings converts the debug configuration and the other
  #   converts the release configuration.  I don't know which is which.
  sed -i -e 's/RuntimeLibrary="5"/RuntimeLibrary="3"/g;
             s/RuntimeLibrary="4"/RuntimeLibrary="2"/g;' gtest/msvc/*.vcproj
else
  set -ex
fi

# TODO(kenton):  Remove the ",no-obsolete" part and fix the resulting warnings.
autoreconf -f -i -Wall,no-obsolete

rm -rf autom4te.cache config.h.in~
exit 0
