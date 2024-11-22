#!/bin/bash
# Runs tests that are defined in the protobuf crate using Cargo.
# This is not a hermetic task because Cargo will fetch the needed
# dependencies from crates.io

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

TMP_DIR=$(mktemp -d)
trap 'rm -rf -- "$TMP_DIR"' EXIT

CARGO_HOME=$TMP_DIR/cargo_home
mkdir $CARGO_HOME

CRATE_ROOT=$TMP_DIR/protobuf
mkdir $CRATE_ROOT

PROTOBUF_TAR=$(rlocation com_google_protobuf/rust/protobuf_crate_dist.tar)

echo "Expanding protobuf_crate tar"
tar -xvf $PROTOBUF_TAR -C $CRATE_ROOT 

CODEGEN_ROOT=$TMP_DIR/protobuf_codegen
mkdir $CODEGEN_ROOT

CODEGEN_TAR=$(rlocation com_google_protobuf/rust/codegen_crate_dist.tar)

if [[ ! -f $CODEGEN_TAR ]]; then
    CODEGEN_TAR=$(rlocation com_google_protobuf/rust/codegen_crate_test.tar)
fi

echo "Expanding codegen_crate tar"
tar -xvf $CODEGEN_TAR -C $CODEGEN_ROOT 

EXAMPLE_ROOT=$TMP_DIR/codegen_example
mkdir $EXAMPLE_ROOT

EXAMPLE_TAR=$(rlocation com_google_protobuf/rust/codegen_example_test.tar)

echo "Expanding codegen_example tar"
tar -xvf $EXAMPLE_TAR -C $EXAMPLE_ROOT 

cd $CRATE_ROOT
# Run all tests except doctests
CARGO_HOME=$CARGO_HOME cargo test --lib --bins --tests

cd $CODEGEN_ROOT
CARGO_HOME=$CARGO_HOME cargo test --lib --bins --tests

cd $EXAMPLE_ROOT
CARGO_HOME=$CARGO_HOME cargo test
