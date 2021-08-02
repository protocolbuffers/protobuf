#!/bin/bash -eu
#
# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -ex

# Install the latest version of Bazel.
if [ -x "$(command -v use_bazel.sh)" ]; then
  use_bazel.sh 4.1.0
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
  CC=gcc bazel test -c opt --test_output=errors ... -- -benchmarks:benchmark
  if [[ $(uname) = "Linux" ]]; then
    CC=gcc bazel test --test_output=errors ...
    CC=gcc bazel test --test_output=errors ... --//:fasttable_enabled=true -- -cmake:test_generated_files -benchmarks:benchmark
  fi
  # TODO: work through these errors and enable this.
  # if gcc -fanalyzer -x c /dev/null -c -o /dev/null; then
  #   CC=gcc bazel test --copt=-fanalyzer --test_output=errors ...
  # fi
fi

if which clang; then
  if [[ $(uname) = "Linux" ]]; then
    CC=clang bazel test --test_output=errors ...
    CC=clang bazel test --test_output=errors -c opt ... -- -benchmarks:benchmark
    CC=clang bazel test --test_output=errors ... --//:fasttable_enabled=true -- -cmake:test_generated_files

    CC=clang bazel test --test_output=errors --config=m32 ... -- -benchmarks:benchmark
    CC=clang bazel test --test_output=errors --config=asan ... -- -benchmarks:benchmark

    # TODO: update to a newer Lua that hopefully does not trigger UBSAN.
    CC=clang bazel test --test_output=errors --config=ubsan ... -- -tests/bindings/lua:test_lua
  fi
fi

if which valgrind; then
  bazel test --config=valgrind ... -- -tests:test_conformance_upb -cmake:cmake_build
fi
