#!/bin/bash
# Runs tests that are defined in the protobuf crate using Cargo. This also
# performs a publish dry-run, which catches some but not all issues that
# would prevent the crates from being published.
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

PROTOBUF_TAR=$(rlocation com_google_protobuf/rust/release_crates/protobuf/protobuf_crate.tar)

echo "Expanding protobuf crate tar"
tar -xvf $PROTOBUF_TAR -C $CRATE_ROOT

CODEGEN_ROOT=$TMP_DIR/protobuf_codegen
mkdir $CODEGEN_ROOT

CODEGEN_TAR=$(rlocation com_google_protobuf/rust/release_crates/protobuf_codegen/protobuf_codegen_crate.tar)

echo "Expanding protobuf_codegen crate tar"
tar -xvf $CODEGEN_TAR -C $CODEGEN_ROOT

EXAMPLE_ROOT=$TMP_DIR/protobuf_example
mkdir $EXAMPLE_ROOT

EXAMPLE_TAR=$(rlocation com_google_protobuf/rust/release_crates/protobuf_example/protobuf_example_crate.tar)

echo "Expanding protobuf_example crate tar"
tar -xvf $EXAMPLE_TAR -C $EXAMPLE_ROOT

WELL_KNOWN_TYPES_ROOT=$TMP_DIR/protobuf_well_known_types
mkdir $WELL_KNOWN_TYPES_ROOT

WELL_KNOWN_TYPES_TAR=$(rlocation com_google_protobuf/rust/release_crates/protobuf_well_known_types/crate.tar)

echo "Expanding protobuf_well_known_types crate tar"
tar -xvf $WELL_KNOWN_TYPES_TAR -C $WELL_KNOWN_TYPES_ROOT

# Put the Bazel-built protoc and plugin at the beginning of $PATH
PATH=$(dirname $(rlocation com_google_protobuf/protoc)):$PATH
PATH=$(dirname $(rlocation com_google_protobuf/upb_generator/minitable/protoc-gen-upb_minitable)):$PATH

cd $CRATE_ROOT
CARGO_HOME=$CARGO_HOME cargo test
CARGO_HOME=$CARGO_HOME cargo publish --dry-run

cd $CODEGEN_ROOT
CARGO_HOME=$CARGO_HOME cargo test
CARGO_HOME=$CARGO_HOME cargo publish --dry-run

cd $EXAMPLE_ROOT
CARGO_HOME=$CARGO_HOME cargo test

cd $WELL_KNOWN_TYPES_ROOT
CARGO_HOME=$CARGO_HOME cargo test

# TODO: Cannot enable this dry-run yet because it checks that the versions of
# its dependencies are published on crates.io, which they are definitely not
# in this case.
# CARGO_HOME=$CARGO_HOME cargo publish --dry-run
