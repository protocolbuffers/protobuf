#!/bin/bash

# Run this script to regenerate descriptor.pbobjc.{h,m} after the protocol
# compiler changes.

# HINT:  Flags passed to generate_descriptor_proto.sh will be passed directly
#   to make when building protoc.  This is particularly useful for passing
#   -j4 to run 4 jobs simultaneously.

set -eu

readonly ScriptDir=$(dirname "$(echo $0 | sed -e "s,^\([^/]\),$(pwd)/\1,")")
readonly ProtoRootDir="${ScriptDir}/.."

pushd "${ProtoRootDir}" > /dev/null

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

# Make sure the compiler is current.
cd src
make $@ protoc

declare -a RUNTIME_PROTO_FILES=( \
  google/protobuf/any.proto \
  google/protobuf/api.proto \
  google/protobuf/descriptor.proto \
  google/protobuf/duration.proto \
  google/protobuf/empty.proto \
  google/protobuf/field_mask.proto \
  google/protobuf/source_context.proto \
  google/protobuf/struct.proto \
  google/protobuf/timestamp.proto \
  google/protobuf/type.proto \
  google/protobuf/wrappers.proto)

./protoc --objc_out="${ProtoRootDir}/objectivec" ${RUNTIME_PROTO_FILES[@]}
