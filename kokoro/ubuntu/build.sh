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

# Log the bazel path and version.
which bazel
bazel version

cd $(dirname $0)/../..

if which gcc; then
  gcc --version
  CC=gcc bazel test --test_output=errors ...
fi

if which clang; then
  CC=clang bazel test --test_output=errors ...
  CC=clang bazel test --test_output=errors --config=m32 ...
  CC=clang bazel test --test_output=errors -c opt ...

  if [[ $(uname) = "Linux" ]]; then
    CC=clang bazel test --test_output=errors --config=asan ...

    # TODO: update to a newer Lua that hopefully does not trigger UBSAN.
    CC=clang bazel test --test_output=errors --config=ubsan ... -- -tests/bindings/lua:test_lua
  fi
fi

if which valgrind; then
  bazel test --config=valgrind ... -- -tests:test_conformance_upb -cmake:cmake_build
fi
