#!/bin/bash

# This script verifies that BUILD files and cmake files are in sync with src/Makefile.am

set -eo pipefail

if [ "$(uname)" != "Linux" ]; then
  echo "build_files_updated_unittest only supported on Linux. Skipping..."
  exit 0
fi

# Keep in sync with files needed by update_file_lists.sh
generated_files=(
  "BUILD"
  "cmake/extract_includes.bat.in"
  "cmake/libprotobuf-lite.cmake"
  "cmake/libprotobuf.cmake"
  "cmake/libprotoc.cmake"
  "cmake/tests.cmake"
  "src/Makefile.am"
)

# If we're running in Bazel, use the Bazel-provided temp-dir.
if [ -n "${TEST_TMPDIR}" ]; then
  # Env-var TEST_TMPDIR is set, assume that this is Bazel.
  # Bazel may have opinions whether we are allowed to delete TEST_TMPDIR.
  test_root="${TEST_TMPDIR}/build_files_updated_unittest"
  mkdir "${test_root}"
else
  # Seems like we're not executed by Bazel.
  test_root=$(mktemp -d)
fi

# From now on, fail if there are any unbound variables.
set -u

# Remove artifacts after test is finished.
function cleanup {
  rm -rf "${test_root}"
}
trap cleanup EXIT

# Create golden dir and add snapshot of current state.
golden_dir="${test_root}/golden"
mkdir -p "${golden_dir}/cmake" "${golden_dir}/src"
for file in ${generated_files[@]}; do
  cp "${file}" "${golden_dir}/${file}"
done

# Create test dir, copy current state into it, and execute update script.
test_dir="${test_root}/test"
cp -R "${golden_dir}" "${test_dir}"

cp "update_file_lists.sh" "${test_dir}/update_file_lists.sh"
chmod +x "${test_dir}/update_file_lists.sh"
cd "${test_root}/test"
bash "${test_dir}/update_file_lists.sh"

# Test whether there are any differences
for file in ${generated_files[@]}; do
  diff "${golden_dir}/${file}" "${test_dir}/${file}"
done
