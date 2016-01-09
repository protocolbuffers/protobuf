#!/bin/sh

# Run this script to generate the configure script and other files that will
# be included in the distribution.  These files are not checked in because they
# are automatically generated.

set -e

if [ ! -z "$@" ]; then
  for argument in "$@"; do
    case $argument in
	  # make curl silent
      "-s")
        curlopts="-s"
        ;;
    esac
  done
fi


# Check that we're being run from the right directory.
if test ! -f src/google/protobuf/stubs/common.h; then
  cat >&2 << __EOF__
Could not find source code.  Make sure you are running this script from the
root of the distribution tree.
__EOF__
  exit 1
fi

# Check that gmock is present.  Usually it is already there since the
# directory is set up as an SVN external.
if test ! -e gmock; then
  echo "Google Mock not present.  Fetching gmock-1.7.0 from the web..."
  curl $curlopts -O https://googlemock.googlecode.com/files/gmock-1.7.0.zip
  unzip -q gmock-1.7.0.zip
  rm gmock-1.7.0.zip
  mv gmock-1.7.0 gmock
fi

set -ex

# TODO(kenton):  Remove the ",no-obsolete" part and fix the resulting warnings.
autoreconf -f -i -Wall,no-obsolete

rm -rf autom4te.cache config.h.in~
exit 0
