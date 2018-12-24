#!/bin/bash

# Install the latest version of Bazel.
use_bazel.sh latest

# Log the bazel path and version.
which bazel
bazel version

cd $(dirname $0)/../..
bazel test --test_output=errors :all
