#!/bin/bash

# Run this script to regenerate *.pbobjc.{h,m} for the well known types after
# the protocol compiler changes.

# HINT:  Flags passed to generate_well_known_types.sh will be passed directly
#   to make when building protoc.  This is particularly useful for passing
#   -j4 to run 4 jobs simultaneously.

set -eu

readonly ScriptDir=$(dirname "$(echo $0 | sed -e "s,^\([^/]\),$(pwd)/\1,")")
readonly ObjCDir="${ScriptDir}"
readonly ProtoRootDir="${ObjCDir}/.."

# Flag for continuous integration to check that everything is current.
CHECK_ONLY=0
if [[ $# -ge 1 && ( "$1" == "--check-only" ) ]] ; then
  CHECK_ONLY=1
  shift
fi

cd "${ProtoRootDir}"

if [[ ! -e src/google/protobuf/stubs/common.h ]]; then
  cat >&2 << __EOF__
Could not find source code.  Make sure you are running this script from the
root of the distribution tree.
__EOF__
  exit 1
fi

if [[ ! -e src/Makefile ]]; then
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

declare -a OBJC_EXTENSIONS=( .pbobjc.h .pbobjc.m )

# Generate to a temp directory to see if they match.
TMP_DIR=$(mktemp -d)
trap "rm -rf ${TMP_DIR}" EXIT
./protoc --objc_out="${TMP_DIR}" ${RUNTIME_PROTO_FILES[@]}

DID_COPY=0
for PROTO_FILE in "${RUNTIME_PROTO_FILES[@]}"; do
  DIR=${PROTO_FILE%/*}
  BASE_NAME=${PROTO_FILE##*/}
  # Drop the extension
  BASE_NAME=${BASE_NAME%.*}
  OBJC_NAME=$(echo "${BASE_NAME}" | awk -F _ '{for(i=1; i<=NF; i++) printf "%s", toupper(substr($i,1,1)) substr($i,2);}')

  for EXT in "${OBJC_EXTENSIONS[@]}"; do
    if ! diff "${ObjCDir}/GPB${OBJC_NAME}${EXT}" "${TMP_DIR}/${DIR}/${OBJC_NAME}${EXT}" > /dev/null 2>&1 ; then
      if [[ "${CHECK_ONLY}" == 1 ]] ; then
        echo "ERROR: The WKTs need to be regenerated! Run $0"
        exit 1
      fi

      echo "INFO: Updating GPB${OBJC_NAME}${EXT}"
      cp "${TMP_DIR}/${DIR}/${OBJC_NAME}${EXT}" "${ObjCDir}/GPB${OBJC_NAME}${EXT}"
      DID_COPY=1
    fi
  done
done

if [[ "${DID_COPY}" == 0 ]]; then
  echo "INFO: Generated source for WellKnownTypes is current."
  exit 0
fi
