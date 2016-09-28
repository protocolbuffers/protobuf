#!/bin/bash

if [ $# -ne 1 ]; then
  cat <<EOF
Usage: $0 <VERSION_NUMBER>

Example:
  $ $0 3.0.0

This script will download pre-built protoc binaries from maven repository and
create the Google.Protobuf.Tools package. Well-known type .proto files will also
be included.
EOF
  exit 1
fi

VERSION_NUMBER=$1
# <directory name> <binary file name> pairs.
declare -a FILE_NAMES=(          \
  windows_x86 windows-x86_32.exe \
  windows_x64 windows-x86_64.exe \
  macosx_x86  osx-x86_32.exe     \
  macosx_x64  osx-x86_64.exe     \
  linux_x86   linux-x86_32.exe   \
  linux_x64   linux-x86_64.exe   \
)

set -e

mkdir -p protoc
# Create a zip file for each binary.
for((i=0;i<${#FILE_NAMES[@]};i+=2));do
  DIR_NAME=${FILE_NAMES[$i]}
  mkdir -p protoc/$DIR_NAME

  if [ ${DIR_NAME:0:3} = "win" ]; then
    TARGET_BINARY="protoc.exe"
  else
    TARGET_BINARY="protoc"
  fi

  BINARY_NAME=${FILE_NAMES[$(($i+1))]}
  BINARY_URL=http://repo1.maven.org/maven2/com/google/protobuf/protoc/${VERSION_NUMBER}/protoc-${VERSION_NUMBER}-${BINARY_NAME}

  if ! wget ${BINARY_URL} -O protoc/$DIR_NAME/$TARGET_BINARY &> /dev/null; then
    echo "[ERROR] Failed to download ${BINARY_URL}" >&2
    echo "[ERROR] Skipped $protoc-${VERSION_NAME}-${DIR_NAME}" >&2
    continue
  fi
done

nuget pack Google.Protobuf.Tools.nuspec
