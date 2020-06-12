#!/bin/bash

set -ex

# Install the latest version of Bazel.
if [ -x "$(command -v use_bazel.sh)" ]; then
  use_bazel.sh latest
fi

# Verify/query CMake
echo PATH=$PATH
ls -l `which cmake`
cmake --version
echo CC=${CC:-cc}
${CC:-cc} --version

# Log the bazel path and version.
which bazel
bazel version

cd $(dirname $0)/../..
bazel test --test_output=errors :all

if [[ $(uname) = "Linux" ]]; then
  # Verify the ASAN build.  Have to exclude test_conformance_upb as protobuf
  # currently leaks memory in the conformance test runner.
  bazel test --copt=-fsanitize=address --linkopt=-fsanitize=address --test_output=errors :all

  # Verify the UBSan build. Have to exclude Lua as the version we are using
  # fails some UBSan tests.

  # For some reason kokoro doesn't have Clang available right now.
  #CC=clang CXX=clang++ bazel test -c dbg --copt=-fsanitize=undefined --copt=-fno-sanitize=function,vptr --linkopt=-fsanitize=undefined --action_env=UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 -- :all -:test_lua

fi

if which valgrind; then
  bazel test --run_under='valgrind --leak-check=full --error-exitcode=1' :all -- -:test_conformance_upb -:cmake_build
fi
