#!/bin/bash

set -ex

# Change to the script's directory
cd $(dirname $0)

# Download 3.0.0 version of protoc
PROTOC_BINARY_NAME="protoc-3.0.0-linux-x86_64.exe"
if [ `uname` = "Darwin" ]; then
  PROTOC_BINARY_NAME="protoc-3.0.0-osx-x86_64.exe"
fi
wget http://repo1.maven.org/maven2/com/google/protobuf/protoc/3.0.0/${PROTOC_BINARY_NAME} -O protoc
chmod +x protoc

# Run tests
rake test
