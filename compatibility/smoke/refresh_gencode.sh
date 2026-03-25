#!/usr/bin/bash
# Run this script to update the gencode for all version subdirs.
# Answer yes when prompted about removing write-protected files.

set -e -x

build="BUILD.bazel"

# Copy and eventually restore the original build file.
cp "${build}" "${build}".original
trap "mv ${build}.original ${build}" EXIT

# Update the subdirs.
for file in v*; do
	add_gencode.sh "${file:1}"
done
