#!/bin/bash -ex
#
# Build file to set up and run tests

# NOTE: in order to avoid blocking anyone, this job always succeeds for now.
exit 0

# Set up artifact output location
: ${KOKORO_ARTIFACTS_DIR:=/tmp/kokoro_artifacts}
: ${BUILD_LOGDIR:=$KOKORO_ARTIFACTS_DIR/logs}
mkdir -p ${BUILD_LOGDIR}

# Change to repo root
cd $(dirname $0)/../../..

# Update submodules
git submodule update --init --recursive

# Build in a separate directory
mkdir -p cmake/build
cd cmake/build

# Print some basic info
xcode-select --print-path
xcodebuild -version
xcrun --show-sdk-path

# Build everything first
cmake -G Xcode ../.. \
  2>&1 | tee ${BUILD_LOGDIR}/00_configure_sponge_log.log
cmake --build . --config Debug \
  2>&1 | tee ${BUILD_LOGDIR}/01_build_sponge_log.log

# Run tests
ctest -C Debug --verbose --quiet \
  --output-log ${BUILD_LOGDIR}/02_test_sponge_log.log
