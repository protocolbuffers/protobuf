#!/bin/bash

MY_DIR="$(dirname "$0")"
TEST_SCRIPT=$MY_DIR/tests.sh
BUILD_DIR=/tmp/protobuf

# Set value used in tests.sh.
PARALLELISM=-j8

set -x  # display all commands

rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR
git clone /var/local/jenkins/protobuf
cd protobuf

# If protoc fails to build, we can't test anything else.
$TEST_SCRIPT cpp || exit 1

# Other tests can fail and we keep on going.
$TEST_SCRIPT java_jdk6
$TEST_SCRIPT java_jdk7
