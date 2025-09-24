#!/bin/bash
# Runs tests that are defined in the protobuf crate using Cargo. This also
# performs a publish dry-run, which catches some but not all issues that
# would prevent the crates from being published.
# This is not a hermetic task because Cargo will fetch the needed
# dependencies from crates.io

# --- begin runfiles.bash initialization ---
# Copy-pasted from Bazel's Bash runfiles library (tools/bash/runfiles/runfiles.bash).
set -euxo pipefail
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

WORKSPACE_ROOT=$TMP_DIR/workspace
mkdir $WORKSPACE_ROOT

WORKSPACE_TAR=$(rlocation com_google_protobuf/rust/release_crates/workspace.tar)

echo "Expanding Cargo workspace tar"

# The tar binary on Windows does not know how to handle file paths that start
# with C:\ or similar, so we go a little out of our way here to avoid passing
# any paths as arguments to tar.
pushd $WORKSPACE_ROOT
tar -xv < $WORKSPACE_TAR
popd

# Put the Bazel-built protoc at the beginning of $PATH
PATH=$(dirname $(rlocation com_google_protobuf/protoc)):$PATH

export RUSTFLAGS="-Dmismatched-lifetime-syntaxes"

cd $WORKSPACE_ROOT
CARGO_HOME=$CARGO_HOME cargo test
CARGO_HOME=$CARGO_HOME cargo publish --dry-run --workspace
