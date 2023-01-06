#!/bin/bash
# This file should be executed with ruby

set -ex

# --- begin runfiles.bash initialization ---
# Copy-pasted from Bazel's Bash runfiles library (tools/bash/runfiles/runfiles.bash).
set -euo pipefail
if [[ ! -d "${RUNFILES_DIR:-/dev/null}" && ! -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  if [[ -f "$0.runfiles_manifest" ]]; then
    export RUNFILES_MANIFEST_FILE="$0.runfiles_manifest"
  elif [[ -f "$0.runfiles/MANIFEST" ]]; then
    export RUNFILES_MANIFEST_FILE="$0.runfiles/MANIFEST"
  elif [[ -f "$0.runfiles/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
    export RUNFILES_DIR="$0.runfiles"
  fi
fi
if [[ -f "${RUNFILES_DIR:-/dev/null}/bazel_tools/tools/bash/runfiles/runfiles.bash" ]]; then
  source "${RUNFILES_DIR}/bazel_tools/tools/bash/runfiles/runfiles.bash"
elif [[ -f "${RUNFILES_MANIFEST_FILE:-/dev/null}" ]]; then
  source "$(grep -m1 "^bazel_tools/tools/bash/runfiles/runfiles.bash " \
          "$RUNFILES_MANIFEST_FILE" | cut -d ' ' -f 2-)"
else
echo >&2 "ERROR: cannot find @bazel_tools//tools/bash/runfiles:runfiles.bash"
exit 1
fi
# --- end runfiles.bash initialization ---

WORKING_DIR=$(dirname $(readlink $BASH_SOURCE))

# rvm use ruby-3.0
# Move all files to the correct directory
cp "$(rlocation src/google/protobuf/*.rb)" $WORKING_DIR/lib/google/protobuf
UTF8_DIR=$WORKING_DIR/ext/google/protobuf_c/third_party/utf8_range
mkdir -p $UTF8_DIR
cp $WORKING_DIR/external/utf8_range/LICENSE $UTF8_DIR/LICENSE
cp $WORKING_DIR/external/utf8_range/naive.c $UTF8_DIR
cp $WORKING_DIR/external/utf8_range/range2-neon.c $UTF8_DIR
cp $WORKING_DIR/external/utf8_range/range2-sse.c $UTF8_DIR
cp $WORKING_DIR/external/utf8_range/utf8_range.h $UTF8_DIR

# Make all files global readable/writable/executable
cd $WORKING_DIR
chmod -R 777 ./

# Build gem
gem build google-protobuf.gemspec
