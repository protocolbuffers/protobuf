#!/bin/bash -u

# This script generates file lists from Bazel for CMake.

set -e

bazel build //pkg:gen_src_file_lists
cp -v bazel-bin/pkg/src_file_lists.cmake src/file_lists.cmake
