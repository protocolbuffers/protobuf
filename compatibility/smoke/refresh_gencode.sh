#!/usr/bin/bash
# Run this script to update the gencode for all version subdirs.
# Answer yes when prompted about removing write-protected files.

set -e -x

build="BUILD.bazel"

# Copy the original build file.
cp "${build}" "${build}".original

# Update the subdirs.
for file in v*; do
	add_gencode.sh "${file:1}"
done

# Restore the original build file.
trap "mv ${build}.original ${build}" EXIT
