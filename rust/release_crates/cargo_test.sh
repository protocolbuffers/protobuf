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
mkdir "$CARGO_HOME"

WORKSPACE_ROOT=$TMP_DIR/workspace
mkdir "$WORKSPACE_ROOT"

WORKSPACE_TAR=$(rlocation com_google_protobuf/rust/release_crates/workspace.tar)

echo "Expanding Cargo workspace tar"

# The tar binary on Windows does not know how to handle file paths that start
# with C:\ or similar, so we go a little out of our way here to avoid passing
# any paths as arguments to tar.
pushd "$WORKSPACE_ROOT"
tar -xv < "$WORKSPACE_TAR"
popd

# Put the Bazel-downloaded Rust toolchain and Bazel-built protoc at the
# beginning of $PATH.
RUST_BIN_DIR="$(dirname "$(rlocation "${CARGO_RLOCATION}")")"
PATH="${RUST_BIN_DIR}:$(dirname "$(rlocation com_google_protobuf/protoc)"):${PATH}"

rustc -vV
cargo -V

EXPECTED_RUST_VERSION="${KOKORO_RUST_VERSION:-${1:-}}"
if [[ -n "${EXPECTED_RUST_VERSION}" ]]; then
  ACTUAL_RUSTC_VERSION="$(rustc -V | tr -d '\r' | awk '{print $2}')"
  if [[ "${ACTUAL_RUSTC_VERSION}" != "${EXPECTED_RUST_VERSION}" && "${ACTUAL_RUSTC_VERSION}" != "${EXPECTED_RUST_VERSION}."* ]]; then
    echo >&2 "ERROR: Expected rustc version ${EXPECTED_RUST_VERSION}, got ${ACTUAL_RUSTC_VERSION}"
    exit 1
  fi
  ACTUAL_CARGO_VERSION="$(cargo -V | tr -d '\r' | awk '{print $2}')"
  if [[ "${ACTUAL_CARGO_VERSION}" != "${EXPECTED_RUST_VERSION}" && "${ACTUAL_CARGO_VERSION}" != "${EXPECTED_RUST_VERSION}."* ]]; then
    echo >&2 "ERROR: Expected cargo version ${EXPECTED_RUST_VERSION}, got ${ACTUAL_CARGO_VERSION}"
    exit 1
  fi
fi

RUSTFLAGS="-Dwarnings"

# On Linux with Clang, `google_protobuf/build.rs` compiles `upb` C sources with
# `-flto=thin`. Route linking through the container's `clang` and `lld` rather
# than Rust's bundled `rust-lld` so Clang and Rust LLVM major versions are
# decoupled (leaving Windows MSVC untouched).
if [[ "$(uname -s)" == "Linux" && "${CC:-}" == clang* ]]; then
  HOST_TRIPLE="$(rustc -vV | sed -n 's/^host: //p')"
  TRIPLE_UPPER="$(echo "${HOST_TRIPLE}" | tr '[:lower:]-' '[:upper:]_')"
  export "CARGO_TARGET_${TRIPLE_UPPER}_LINKER=${CC}"
  RUSTFLAGS="${RUSTFLAGS} -Clink-arg=-fuse-ld=lld"
fi
export RUSTFLAGS

cd "$WORKSPACE_ROOT"
CARGO_HOME=$CARGO_HOME cargo test
CARGO_HOME=$CARGO_HOME cargo publish --dry-run --workspace
