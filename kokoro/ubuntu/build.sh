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
