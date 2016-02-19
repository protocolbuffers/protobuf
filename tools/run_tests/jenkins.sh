#!/bin/bash

MY_DIR="$(dirname "$0")"
TEST_SCRIPT=$MY_DIR/tests.sh
BUILD_DIR=/tmp/protobuf

# Set value used in tests.sh.
PARALLELISM=-j8

set -e  # exit immediately on error
set -x  # display all commands

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR
git clone /var/local/jenkins/protobuf
cd protobuf

OUTPUT_DIR=`mktemp -d`
mkdir -p $OUTPUT_DIR/1

# cpp build needs to run first, non-parallelized, so that protoc is available
# for other builds.
$TEST_SCRIPT cpp | tee $OUTPUT_DIR/1/cpp

# Other tests are run in parallel.  The overall run fails if any one of them
# fails.

parallel $TEST_SCRIPT ::: \
  java_jdk7 \
  javanano_jdk7 \
  python \
  python_cpp

# java_jdk6 \
