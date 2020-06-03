#!/bin/bash

# Install the latest version of Bazel.
use_bazel.sh latest

# Verify/query CMake
echo PATH=$PATH
ls -l `which cmake`
cmake --version

# Log the bazel path and version.
which bazel
bazel version

cd $(dirname $0)/../..
bazel test --test_output=errors :all

if [[ $(uname) = "Linux" ]]; then
  # Verify the ASAN build.  Have to exclude test_conformance_upb as protobuf
  # currently leaks memory in the conformance test runner.
  bazel test --copt=-fsanitize=address --linkopt=-fsanitize=address --test_output=errors -- :all -:test_conformance_upb

  # Verify the UBSan build. Have to exclude Lua as the version we are using
  # fails some UBSan tests.

  # For some reason kokoro doesn't have Clang available right now.
  #CC=clang CXX=clang++ bazel test -c dbg --copt=-fsanitize=undefined --copt=-fno-sanitize=function,vptr --linkopt=-fsanitize=undefined --action_env=UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 -- :all -:test_lua
fi
