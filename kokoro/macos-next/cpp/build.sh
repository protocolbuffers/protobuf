#!/bin/bash -ex
#
# Build file to set up and run tests

set -o pipefail

# Set up artifact output location
: ${KOKORO_ARTIFACTS_DIR:=/tmp/kokoro_artifacts}
: ${BUILD_LOGDIR:=$KOKORO_ARTIFACTS_DIR/logs}
mkdir -p ${BUILD_LOGDIR}

pwd
ls -l /
ls -l /tmpfs
ls -l /Volumes/BuildData

# Change to repo root
cd $(dirname $0)/../../..

ls -l $(which cmake)

function recursive-ls() {
  [[ $1 == / ]] && return
  recursive-ls $(dirname $1) || true
  if [[ -d $1 ]]; then
    echo D $1
  elif [[ -f $1 ]]; then
    echo F $1
  elif [[ -h $1 ]]; then
    echo L $1
  elif [[ -e $1 ]]; then
    echo "  $1"
  else
    echo "Not found: $1"
    return 1
  fi
}
set +x
recursive-ls /usr/local/Cellar/cmake/3.22.2/share/cmake/Modules/CMakeCXXCompilerABI.cpp
#recursive-ls /Volumes/BuildData/usr/local/Cellar/cmake/3.22.2/share/cmake/Modules/CMakeCXXCompilerABI.cpp || true
set -x

cat > $TEMP/print-all.cmake <<EOF
get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_vN \${_variableNames})
    message(STATUS "\${_vN}=\${\${_vN}}")
endforeach()
EOF

cmake -P $TEMP/print-all.cmake

CMAKE_ROOT=/usr/local/Cellar/cmake/3.22.2/share/cmake \
  cmake -P $TEMP/print-all.cmake

# Update submodules
git submodule update --init --recursive

# Build in a separate directory
mkdir -p cmake/build
cd cmake/build

# Print some basic info
xcode-select --print-path
xcodebuild -version
xcodebuild -showsdks
xcrun --show-sdk-path

sudo xcode-select -s /Applications/Xcode_13.2.1.app

# Print some basic info
xcode-select --print-path
xcodebuild -version
xcodebuild -showsdks
xcrun --show-sdk-path

#  -DHAVE_ZLIB
#  -isysroot /Applications/Xcode_13.2.1.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk \

#  -I/tmpfs/src/github/protobuf/third_party/googletest/googlemock/include \
#  -isystem /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.1.sdk/usr/include \

# echo "#include <cmath>" | \
#   /Applications/Xcode_13.2.1.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang \
#   -v \
#   -x c++ \
#   -target x86_64-apple-macos11.4 \
#   -fmessage-length\=0 \
#   -fdiagnostics-show-note-include-stack \
#   -fmacro-backtrace-limit\=0 \
#   -Wno-trigraphs \
#   -fpascal-strings \
#   -O0 \
#   -Wno-missing-field-initializers \
#   -Wno-missing-prototypes \
#   -Wno-return-type \
#   -Wno-non-virtual-dtor \
#   -Wno-overloaded-virtual \
#   -Wno-exit-time-destructors \
#   -Wno-missing-braces \
#   -Wparentheses \
#   -Wswitch \
#   -Wno-unused-function \
#   -Wno-unused-label \
#   -Wno-unused-parameter \
#   -Wno-unused-variable \
#   -Wunused-value \
#   -Wno-empty-body \
#   -Wno-uninitialized \
#   -Wno-unknown-pragmas \
#   -Wno-shadow \
#   -Wno-four-char-constants \
#   -Wno-conversion \
#   -Wno-constant-conversion \
#   -Wno-int-conversion \
#   -Wno-bool-conversion \
#   -Wno-enum-conversion \
#   -Wno-float-conversion \
#   -Wno-non-literal-null-conversion \
#   -Wno-objc-literal-conversion \
#   -Wno-shorten-64-to-32 \
#   -Wno-newline-eof \
#   -Wno-c++11-extensions \
#   -DCMAKE_INTDIR\=\"Debug\" \
#   -DGOOGLE_PROTOBUF_CMAKE_BUILD \
#   -DHAVE_ZLIB \
#   -fasm-blocks \
#   -fstrict-aliasing \
#   -Wdeprecated-declarations \
#   -Winvalid-offsetof \
#   -g \
#   -Wno-sign-conversion \
#   -Wno-infinite-recursion \
#   -Wno-move \
#   -Wno-comma \
#   -Wno-block-capture-autoreleasing \
#   -Wno-strict-prototypes \
#   -Wno-range-loop-analysis \
#   -Wno-semicolon-before-method-body \
#   -I/tmpfs/src/github/protobuf/cmake/build/Debug/include \
#   -I/tmpfs/src/github/protobuf/cmake/build \
#   -I/tmpfs/src/github/protobuf/src \
#   -I/tmpfs/src/github/protobuf/third_party/googletest/googlemock \
#   -I/tmpfs/src/github/protobuf/third_party/googletest/googletest \
#   -I/tmpfs/src/github/protobuf/third_party/googletest/googletest/include \
#   -I/tmpfs/src/github/protobuf/third_party/googletest/googlemock/include \
#   -I/tmpfs/src/github/protobuf/cmake/build/protobuf.build/Debug/libprotobuf.build/DerivedSources-normal/x86_64 \
#   -I/tmpfs/src/github/protobuf/cmake/build/protobuf.build/Debug/libprotobuf.build/DerivedSources/x86_64 \
#   -I/tmpfs/src/github/protobuf/cmake/build/protobuf.build/Debug/libprotobuf.build/DerivedSources \
#   -F/tmpfs/src/github/protobuf/cmake/build/Debug \
#   -std\=c++11 \
#   -MMD \
#   -MT dependencies \
#   -MF /tmpfs/src/github/protobuf/cmake/build/protobuf.build/Debug/libprotobuf.build/Objects-normal/x86_64/zero_copy_stream_impl_lite.d \
#   --serialize-diagnostics /tmpfs/src/github/protobuf/cmake/build/protobuf.build/Debug/libprotobuf.build/Objects-normal/x86_64/zero_copy_stream_impl_lite.dia \
#   -c - \
#   -o /dev/null

export PATH

which cmake
cmake --version

export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)

# Build everything first
cmake -G Xcode ../.. \
  2>&1 | tee ${BUILD_LOGDIR}/00_configure_sponge_log.log

ls ${BUILD_LOGDIR}

echo "============================================================"
echo -e "CMakeOutput.log:\n"
cat CMakeFiles/CMakeOutput.log

echo "============================================================"
echo -e "CMakeError.log:\n"
cat CMakeFiles/CMakeError.log

echo "============================================================"

cmake --build . --config Debug \
  2>&1 | tee ${BUILD_LOGDIR}/01_build_sponge_log.log

# Run tests
ctest -C Debug --verbose --quiet \
  --output-log ${BUILD_LOGDIR}/02_test_sponge_log.log
