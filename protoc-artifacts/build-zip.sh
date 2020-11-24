#!/bin/bash

if [ $# -ne 2 ]; then
  cat <<EOF
Usage: $0 <TARGET> <VERSION_NUMBER>

TARGET: protoc | protoc-gen-javalite

Example:
  $ $0 protoc 3.0.0
  $ $0 protoc-gen-javalite 3.0.0

This script will download pre-built protoc or protoc plugin binaries from maven
repository and create .zip packages suitable to be included in the github
release page. If the target is protoc, well-known type .proto files will also be
included. Each invocation will create 8 zip packages:
  dist/<TARGET>-<VERSION_NUMBER>-win32.zip
  dist/<TARGET>-<VERSION_NUMBER>-win64.zip
  dist/<TARGET>-<VERSION_NUMBER>-osx-x86_64.zip
  dist/<TARGET>-<VERSION_NUMBER>-linux-x86_32.zip
  dist/<TARGET>-<VERSION_NUMBER>-linux-x86_64.zip
  dist/<TARGET>-<VERSION_NUMBER>-linux-aarch_64.zip
  dist/<TARGET>-<VERSION_NUMBER>-linux-ppcle_64.zip
  dist/<TARGET>-<VERSION_NUMBER>-linux-s390x.zip
EOF
  exit 1
fi

TARGET=$1
VERSION_NUMBER=$2

# <zip file name> <binary file name> pairs.
declare -a FILE_NAMES=( \
  win32.zip windows-x86_32.exe \
  win64.zip windows-x86_64.exe \
  osx-x86_64.zip osx-x86_64.exe \
  linux-x86_32.zip linux-x86_32.exe \
  linux-x86_64.zip linux-x86_64.exe \
  linux-aarch_64.zip linux-aarch_64.exe \
  linux-ppcle_64.zip linux-ppcle_64.exe \
  linux-s390x.zip linux-s390x.exe \
)

# List of all well-known types to be included.
declare -a WELL_KNOWN_TYPES=(           \
  google/protobuf/descriptor.proto      \
  google/protobuf/any.proto             \
  google/protobuf/api.proto             \
  google/protobuf/duration.proto        \
  google/protobuf/empty.proto           \
  google/protobuf/field_mask.proto      \
  google/protobuf/source_context.proto  \
  google/protobuf/struct.proto          \
  google/protobuf/timestamp.proto       \
  google/protobuf/type.proto            \
  google/protobuf/wrappers.proto        \
  google/protobuf/compiler/plugin.proto \
)

set -e

# A temporary working directory to put all files.
DIR=$(mktemp -d)

# Copy over well-known types.
mkdir -p ${DIR}/include/google/protobuf/compiler
for PROTO in ${WELL_KNOWN_TYPES[@]}; do
  cp -f ../src/${PROTO} ${DIR}/include/${PROTO}
done

# Create a readme file.
cat <<EOF > ${DIR}/readme.txt
Protocol Buffers - Google's data interchange format
Copyright 2008 Google Inc.
https://developers.google.com/protocol-buffers/

This package contains a precompiled binary version of the protocol buffer
compiler (protoc). This binary is intended for users who want to use Protocol
Buffers in languages other than C++ but do not want to compile protoc
themselves. To install, simply place this binary somewhere in your PATH.

If you intend to use the included well known types then don't forget to
copy the contents of the 'include' directory somewhere as well, for example
into '/usr/local/include/'.

Please refer to our official github site for more installation instructions:
  https://github.com/protocolbuffers/protobuf
EOF

mkdir -p dist
mkdir -p ${DIR}/bin
# Create a zip file for each binary.
for((i=0;i<${#FILE_NAMES[@]};i+=2));do
  ZIP_NAME=${FILE_NAMES[$i]}
  if [ ${ZIP_NAME:0:3} = "win" ]; then
    BINARY="$TARGET.exe"
  else
    BINARY="$TARGET"
  fi
  BINARY_NAME=${FILE_NAMES[$(($i+1))]}
  BINARY_URL=https://repo1.maven.org/maven2/com/google/protobuf/$TARGET/${VERSION_NUMBER}/$TARGET-${VERSION_NUMBER}-${BINARY_NAME}
  if ! wget ${BINARY_URL} -O ${DIR}/bin/$BINARY &> /dev/null; then
    echo "[ERROR] Failed to download ${BINARY_URL}" >&2
    echo "[ERROR] Skipped $TARGET-${VERSION_NAME}-${ZIP_NAME}" >&2
    continue
  fi
  TARGET_ZIP_FILE=`pwd`/dist/$TARGET-${VERSION_NUMBER}-${ZIP_NAME}
  pushd $DIR &> /dev/null
  chmod +x bin/$BINARY
  if [ "$TARGET" = "protoc" ]; then
    zip -r ${TARGET_ZIP_FILE} include bin readme.txt &> /dev/null
  else
    zip -r ${TARGET_ZIP_FILE} bin &> /dev/null
  fi
  rm  bin/$BINARY
  popd &> /dev/null
  echo "[INFO] Successfully created ${TARGET_ZIP_FILE}"
done
