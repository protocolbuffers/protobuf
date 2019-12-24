#!/bin/bash

# Run this script to regenerate *.pbobjc.{h,m} for the well known types after
# the protocol compiler changes.

# HINT:  Flags passed to generate_well_known_types.sh will be passed directly
#   to make when building protoc.  This is particularly useful for passing
#   -j4 to run 4 jobs simultaneously.

set -eu

readonly ScriptDir=$(dirname "$(echo $0 | sed -e "s,^\([^/]\),$(pwd)/\1,")")
readonly ProtoRootDir="${ScriptDir}/.."

# Flag for continuous integration to check that everything is current.
CHECK_ONLY=0
if [[ $# -ge 1 && ( "$1" == "--check-only" ) ]] ; then
  CHECK_ONLY=1
  shift
fi

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
  google/protobuf/duration.proto \
  google/protobuf/empty.proto \
  google/protobuf/field_mask.proto \
  google/protobuf/source_context.proto \
  google/protobuf/struct.proto \
  google/protobuf/timestamp.proto \
  google/protobuf/type.proto \
  google/protobuf/wrappers.proto)

# Generate to a temp directory to see if they match.
TMP_DIR=$(mktemp -d)
trap "rm -rf ${TMP_DIR}" EXIT
./protoc --objc_out="${TMP_DIR}" ${RUNTIME_PROTO_FILES[@]}
set +e
diff -r "${TMP_DIR}/google" "${ProtoRootDir}/objectivec/google" > /dev/null
if [[ $? -eq 0 ]] ; then
  echo "Generated source for WellKnownTypes is current."
  exit 0
fi
set -e

# If check only mode, error out.
if [[ "${CHECK_ONLY}" == 1 ]] ; then
  echo "ERROR: The WKTs need to be regenerated! Run $0"
  exit 1
fi

# Copy them over.
echo "Copying over updated WellKnownType sources."
cp -r "${TMP_DIR}/google/." "${ProtoRootDir}/objectivec/google/"
