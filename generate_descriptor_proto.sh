#!/bin/sh

# Run this script to regenerate descriptor.pb.{h,cc} after the protocol
# compiler changes.  Since these files are compiled into the protocol compiler
# itself, they cannot be generated automatically by a make rule.  "make check"
# will fail if these files do not match what the protocol compiler would
# generate.
#
# HINT:  Flags passed to generate_descriptor_proto.sh will be passed directly
#   to make when building protoc.  This is particularly useful for passing
#   -j4 to run 4 jobs simultaneously.

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

cd src
make $@ google/protobuf/stubs/pbconfig.h
CORE_PROTO_IS_CORRECT=0
while [ $CORE_PROTO_IS_CORRECT -ne 1 ]
do
  CORE_PROTO_IS_CORRECT=1
  cp google/protobuf/descriptor.pb.h google/protobuf/descriptor.pb.h.tmp
  cp google/protobuf/descriptor.pb.cc google/protobuf/descriptor.pb.cc.tmp
  cp google/protobuf/compiler/plugin.pb.h google/protobuf/compiler/plugin.pb.h.tmp
  cp google/protobuf/compiler/plugin.pb.cc google/protobuf/compiler/plugin.pb.cc.tmp

  make $@ protoc &&
    ./protoc --cpp_out=dllexport_decl=LIBPROTOBUF_EXPORT:. google/protobuf/descriptor.proto && \
    ./protoc --cpp_out=dllexport_decl=LIBPROTOC_EXPORT:. google/protobuf/compiler/plugin.proto

  diff google/protobuf/descriptor.pb.h google/protobuf/descriptor.pb.h.tmp > /dev/null
  if test $? -ne 0; then
    CORE_PROTO_IS_CORRECT=0
  fi
  diff google/protobuf/descriptor.pb.cc google/protobuf/descriptor.pb.cc.tmp > /dev/null
  if test $? -ne 0; then
    CORE_PROTO_IS_CORRECT=0
  fi
  diff google/protobuf/compiler/plugin.pb.h google/protobuf/compiler/plugin.pb.h.tmp > /dev/null
  if test $? -ne 0; then
    CORE_PROTO_IS_CORRECT=0
  fi
  diff google/protobuf/compiler/plugin.pb.cc google/protobuf/compiler/plugin.pb.cc.tmp > /dev/null
  if test $? -ne 0; then
    CORE_PROTO_IS_CORRECT=0
  fi

  rm google/protobuf/descriptor.pb.h.tmp
  rm google/protobuf/descriptor.pb.cc.tmp
  rm google/protobuf/compiler/plugin.pb.h.tmp
  rm google/protobuf/compiler/plugin.pb.cc.tmp
done
cd ..
