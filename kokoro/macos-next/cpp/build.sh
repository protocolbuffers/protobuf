#!/bin/bash -ex -o pipefail
#
# Build file to set up and run tests

#
# Set up logging output location
#
: ${KOKORO_ARTIFACTS_DIR:=/tmp/protobuf_test_logs}
: ${BUILD_LOGDIR:=$KOKORO_ARTIFACTS_DIR/logs}
mkdir -p ${BUILD_LOGDIR}
: ${TEST_LOGDIR:=$KOKORO_ARTIFACTS_DIR/tests}
mkdir -p ${TEST_LOGDIR}

#
# Change to repo root
#
if [[ -h /tmpfs ]] && [[ ${PWD} == /tmpfs/src ]]; then
  # Workaround for internal Kokoro bug: b/227401944
  cd /Volumes/BuildData/tmpfs/src/github/protobuf
else
  cd $(dirname $0)/../../..
fi

#
# Update submodules
#
git submodule update --init --recursive

#
# Configure and build in a separate directory
#
mkdir -p cmake/build
cd cmake/build

cmake -G Xcode ../.. -Dprotobuf_TEST_XML_OUTDIR=${TEST_LOGDIR} \
  2>&1 | tee ${BUILD_LOGDIR}/01_configure.log

cp CMakeFiles/CMake*.log ${BUILD_LOGDIR}

cmake --build . --config Debug \
  2>&1 | tee ${BUILD_LOGDIR}/02_build.log

#
# Run tests
#
ctest -C Debug -j4 --verbose \
  --output-log ${BUILD_LOGDIR}/03_test.log

#
# Rename test XML output
#
find ${TEST_LOGDIR} -name '*.xml' | while read xmllog; do
  mkdir -p ${xmllog%.xml}
  mv -v ${xmllog} ${xmllog%.xml}/sponge_log.xml
done
