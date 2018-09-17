#!/bin/bash
#
# This is the script that runs inside Docker, once the image has been built,
# to execute all tests for the "pull request" project.

WORKSPACE_BASE=`pwd`
MY_DIR="$(dirname "$0")"
TEST_SCRIPT=./tests.sh
BUILD_DIR=/tmp/protobuf

set -e  # exit immediately on error
set -x  # display all commands

# The protobuf repository is mounted into our Docker image, but read-only.
# We clone into a directory inside Docker (this is faster than cp).
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR
git clone /var/local/kokoro/protobuf
cd protobuf

# Initialize any submodules:
git submodule update --init --recursive

$TEST_SCRIPT $TEST_SET
