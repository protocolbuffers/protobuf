#!/usr/bin/bash
# Run this script to update the gencode for all version subdirs.
# Answer yes when prompted about removing write-protected files.

build="BUILD.bazel"

# Remember the original length of the build file.
lines=`wc "${build}" | cut -f2 -d' '`

# Update the subdirs.
for file in v*; do
	add_gencode.sh "${file:1}"
done

# Restore the build file to its original size.
orig=`head --lines="${lines}" "${build}"`
echo "${orig}" > "${build}"
