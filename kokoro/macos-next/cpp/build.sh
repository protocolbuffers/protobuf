#!/bin/bash -ex -o pipefail
#
# Build file to set up and run tests

cd /Volumes/BuildData/tmpfs/src

#
# Print some basic info
#
pwd
ls -l /
ls -l /Volumes/BuildData

xcode-select --print-path
xcodebuild -version
xcodebuild -showsdks
xcrun --show-sdk-path

ls -l $(which cmake)
cmake --version

#
# Debugging info: print all default CMake settings.
#
cat > $TEMP/print-all.cmake <<EOF
get_cmake_property(_vars VARIABLES)
list (SORT _vars)
foreach (_var \${_vars})
    message(STATUS "\${_var}=\${\${_var}}")
endforeach()
EOF

cmake -P $TEMP/print-all.cmake

#
# Set up a path for outputs
#
BUILD_LOG_DIR=$KOKORO_ARTIFACTS_DIR/_logs
mkdir -p $BUILD_LOG_DIR

#
# Change to repo root
#
cd $(dirname $0)/../../..

#
# Build in a separate directory
#
mkdir -p cmake/build
cd cmake/build

#
# Configure and stage logs
#
cmake -G Xcode ../.. -Dprotobuf_BUILD_TESTS=OFF

cp CMakeFiles/CMakeError.log $BUILD_LOG_DIR
cp CMakeFiles/CMakeOutput.log $BUILD_LOG_DIR

#
# Build
#
cmake --build . --config Debug \
  2>&1 | tee ${BUILD_LOG_DIR}/01_build.log

#
# Run tests
#
ctest -C Debug --verbose --quiet \
  --output-log ${BUILD_LOG_DIR}/02_test.log
