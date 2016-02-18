#!/bin/bash

MY_DIR="$(dirname "$0")"
BUILD_DIR=/tmp/protobuf

source $MY_DIR/tests.sh

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR
git clone /var/local/jenkins/protobuf
cd protobuf
build_cpp
