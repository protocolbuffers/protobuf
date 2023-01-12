#!/bin/bash
# This file should be executed with jruby

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

# Make a temporary directory and move to it to do all packaging work
mkdir -p tmp
cd tmp

# Move all generated files to lib/google/protobuf
mkdir -p lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/any_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/api_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/descriptor_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/duration_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/empty_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/field_mask_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/source_context_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/struct_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/timestamp_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/type_pb.rb)" lib/google/protobuf
cp "$(rlocation com_google_protobuf/src/google/protobuf/wrappers_pb.rb)" lib/google/protobuf

cp "$(rlocation com_google_protobuf/ruby/lib/google/protobuf_java.jar)" lib/google

# Move gemspec file to current directory
cp "$(rlocation com_google_protobuf/ruby/google.protobuf.gemspec)" .

# Make all files global readable/writable/executable
chmod -R 777 ./

# Build gem
gem build google-protobuf.gemspec
