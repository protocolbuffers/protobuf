#!/bin/sh

# Run this script to regenerate descriptor.pb.{h,cc} after the protocol
# compiler changes.  Since these files are compiled into the protocol compiler
# itself, they cannot be generated automatically by a make rule.  "make check"
# will fail if these files do not match what the protocol compiler would
# generate.

# Note that this will always need to be run once after running
# extract_from_google3.sh.  That script initially copies descriptor.pb.{h,cc}
# over from the google3 code and fixes it up to compile outside of google3, but
# it cannot fix the encoded descriptor embedded in descriptor.pb.cc.  So, once
# the protocol compiler has been built with the slightly-broken
# descriptor.pb.cc, the files must be regenerated and the compiler must be
# built again.

if test ! -e src/google/protobuf/stubs/common.h; then
  cat >&2 << __EOF__
Could not find source code.  Make sure you are running this script from the
root of the distribution tree.
__EOF__
  exit 1
fi

if test ! -e src/Makefile; then
  cat >&2 << __EOF__
Could not find src/Makefile.  You must run ./configure (and perhaps
./autogen.sh) first.
__EOF__
  exit 1
fi

pushd src
make protoc && ./protoc --cpp_out=dllexport_decl=LIBPROTOBUF_EXPORT:. google/protobuf/descriptor.proto
popd
