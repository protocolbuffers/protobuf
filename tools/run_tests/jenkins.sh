#!/bin/bash

WORKSPACE_BASE=`pwd`
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
$TEST_SCRIPT cpp > >(tee $CPP_STDOUT) 2> >(tee $CPP_STDERR >&2)

# Other tests are run in parallel.  The overall run fails if any one of them
# fails.

FAILED=false

parallel --results $LOG_OUTPUT_DIR --joblog $OUTPUT_DIR/joblog $TEST_SCRIPT ::: \
  csharp \
  java_jdk7 \
  javanano_jdk7 \
  java_oracle7 \
  javanano_oracle7 \
  python \
  python_cpp \
  ruby21 \
  || true  # Process test results even if tests fail.

# The directory that is copied from Docker back into the Jenkins workspace.
COPY_FROM_DOCKER=/var/local/git/protobuf/testoutput
mkdir -p $COPY_FROM_DOCKER
TESTOUTPUT_XML_FILE=$COPY_FROM_DOCKER/testresults.xml

python $MY_DIR/../jenkins/make_test_output.py $OUTPUT_DIR > $TESTOUTPUT_XML_FILE

ls -l $TESTOUTPUT_XML_FILE

### disabled tests
# java_jdk6 \
