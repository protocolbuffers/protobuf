#!/bin/bash
# A script that publishes the protobuf crates to crates.io.

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

read -p 'Enter cratesio auth token: ' AUTH_TOKEN

TMP_DIR=$(mktemp -d)
trap 'rm -rf -- "$TMP_DIR"' EXIT

CARGO_HOME=$TMP_DIR/cargo_home
mkdir $CARGO_HOME

CRATE_ROOT=$TMP_DIR/protobuf
mkdir $CRATE_ROOT

PROTOBUF_TAR=$(rlocation com_google_protobuf/rust/release_crates/protobuf/protobuf_crate.tar)

echo "Expanding protobuf crate tar"
tar -xvf $PROTOBUF_TAR -C $CRATE_ROOT 

CODEGEN_ROOT=$TMP_DIR/protobuf_codegen
mkdir $CODEGEN_ROOT

CODEGEN_TAR=$(rlocation com_google_protobuf/rust/release_crates/protobuf_codegen/protobuf_codegen_crate.tar)

echo "Expanding protbuf_codegen crate tar"
tar -xvf $CODEGEN_TAR -C $CODEGEN_ROOT 

EXAMPLE_ROOT=$TMP_DIR/protobuf_example
mkdir $EXAMPLE_ROOT

EXAMPLE_TAR=$(rlocation com_google_protobuf/rust/release_crates/protobuf_example/protobuf_example_crate.tar)

echo "Expanding protobuf_example crate tar"
tar -xvf $EXAMPLE_TAR -C $EXAMPLE_ROOT 

# Put the Bazel-built protoc and plugin at the beginning of $PATH
PATH=$(dirname $(rlocation com_google_protobuf/protoc)):$PATH
PATH=$(dirname $(rlocation com_google_protobuf/upb_generator/minitable/protoc-gen-upb_minitable)):$PATH

cd $CRATE_ROOT
CARGO_HOME=$CARGO_HOME CARGO_REGISTRY_TOKEN=$AUTH_TOKEN cargo publish

cd $CODEGEN_ROOT
CARGO_HOME=$CARGO_HOME CARGO_REGISTRY_TOKEN=$AUTH_TOKEN cargo publish

cd $EXAMPLE_ROOT
CARGO_HOME=$CARGO_HOME CARGO_REGISTRY_TOKEN=$AUTH_TOKEN cargo publish

