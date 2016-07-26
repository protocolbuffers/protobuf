#!/bin/bash

if [ $# -eq 0 ]; then
  cat <<EOF
Usage: $0 <VERSION_NUMBER>

Example:
  $ $0 3.0.0-beta-4

This script will download pre-built protoc binaries from maven repository
and package them with well-known type .proto files to create .zip packages
suitable to be included in the github release page. Each invocation will
create 5 zip packages:
  dist/protoc-<VERSION_NUMBER>-win32.zip
  dist/protoc-<VERSION_NUMBER>-osx-x86_32.zip
  dist/protoc-<VERSION_NUMBER>-osx-x86_64.zip
  dist/protoc-<VERSION_NUMBER>-linux-x86_32.zip
  dist/protoc-<VERSION_NUMBER>-linux-x86_64.zip
EOF
  exit 1
fi

VERSION_NUMBER=$1

# <zip file name> <binary file name> pairs.
declare -a FILE_NAMES=( \
  win32.zip windows-x86_32.exe \
  osx-x86_32.zip osx-x86_32.exe \
  osx-x86_64.zip osx-x86_64.exe \
  linux-x86_32.zip linux-x86_32.exe \
  linux-x86_64.zip linux-x86_64.exe \
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

Please refer to our official github site for more installation instructions:
  https://github.com/google/protobuf
EOF

mkdir -p dist
mkdir -p ${DIR}/bin
# Create a zip file for each binary.
for((i=0;i<${#FILE_NAMES[@]};i+=2));do
  ZIP_NAME=${FILE_NAMES[$i]}
  BINARY_NAME=${FILE_NAMES[$(($i+1))]}
  BINARY_URL=http://repo1.maven.org/maven2/com/google/protobuf/protoc/${VERSION_NUMBER}/protoc-${VERSION_NUMBER}-${BINARY_NAME}
  if ! wget ${BINARY_URL} -O ${DIR}/bin/protoc &> /dev/null; then
    echo "[ERROR] Failed to download ${BINARY_URL}" >&2
    echo "[ERROR] Skipped protoc-${VERSION_NAME}-${ZIP_NAME}" >&2
    continue
  fi
  TARGET_ZIP_FILE=`pwd`/dist/protoc-${VERSION_NUMBER}-${ZIP_NAME}
  pushd $DIR &> /dev/null
  chmod +x bin/protoc
  zip -r ${TARGET_ZIP_FILE} include bin readme.txt &> /dev/null
  popd &> /dev/null
  echo "[INFO] Successfully created ${TARGET_ZIP_FILE}"
done
