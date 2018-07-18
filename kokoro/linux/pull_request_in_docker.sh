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

# Set up the directory where our test output is going to go.
OUTPUT_DIR=`mktemp -d`
LOG_OUTPUT_DIR=$OUTPUT_DIR/logs
mkdir -p $LOG_OUTPUT_DIR/1/cpp

################################################################################
# cpp build needs to run first, non-parallelized, so that protoc is available
# for other builds.

# Output filenames to follow the overall scheme used by parallel, ie:
#  $DIR/logs/1/cpp/stdout
#  $DIR/logs/1/cpp/stderr
#  $DIR/logs/1/csharp/stdout
#  $DIR/logs/1/csharp/stderr
#  $DIR/logs/1/java_jdk7/stdout
#  $DIR/logs/1/java_jdk7/stderr
CPP_STDOUT=$LOG_OUTPUT_DIR/1/cpp/stdout
CPP_STDERR=$LOG_OUTPUT_DIR/1/cpp/stderr

# Time the C++ build, so we can put this info in the test output.
# It's important that we get /usr/bin/time (which supports -f and -o) and not
# the bash builtin "time" which doesn't.
TIME_CMD="/usr/bin/time -f %e -o $LOG_OUTPUT_DIR/1/cpp/build_time"

$TIME_CMD $TEST_SCRIPT cpp > >(tee $CPP_STDOUT) 2> >(tee $CPP_STDERR >&2)

# Other tests are run in parallel. TEST_SET is defined in
# buildcmds/pull_request{_32}.sh

parallel --results $LOG_OUTPUT_DIR --joblog $OUTPUT_DIR/joblog $TEST_SCRIPT ::: \
  $TEST_SET \
  || FAILED="true"  # Process test results even if tests fail.

cat $OUTPUT_DIR/joblog

# The directory that is copied from Docker back into the Kokoro workspace.
COPY_FROM_DOCKER=/var/local/git/protobuf/testoutput
mkdir -p $COPY_FROM_DOCKER
TESTOUTPUT_XML_FILE=$COPY_FROM_DOCKER/sponge_log.xml

# Process all the output files from "parallel" and package them into a single
# .xml file with detailed, broken-down test output.
python $MY_DIR/make_test_output.py $OUTPUT_DIR > $TESTOUTPUT_XML_FILE

ls -l $TESTOUTPUT_XML_FILE

if [ "$FAILED" == "true" ]; then
	exit 1
fi
