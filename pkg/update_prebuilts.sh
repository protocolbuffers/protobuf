#!/usr/bin/env bash
#
# Builds a local prebuilt protoc zip archive for the current platform and
# updates bazel/private/oss/toolchains/prebuilt/tool_integrity.bzl in place with
# a file:// URL and SHA-256 hash pointing to it.
#
# Usage:
#   bazel run //pkg:update_prebuilts   (or `bazel run :update_prebuilts`)
#
# Relationship to compute_integrity.sh:
#   During official releases, the Rapid release pipeline builds prebuilt protoc
#   archives across all supported platforms and runs compute_integrity.sh to
#   populate tool_integrity.bzl with https://github.com/.../releases/download/
#   URLs and SHA-256 hashes for every release archive.
#
#   On development branches (or before a release is published to GitHub),
#   tool_integrity.bzl is checked in with an empty RELEASED_BINARY_INTEGRITY
#   dict. Running this target populates tool_integrity.bzl with a single-platform
#   file:// entry backed by .prebuilt_protoc/ so that prebuilt protoc toolchain
#   resolution and authenticity validation can be tested locally and in CI.

set -euo pipefail

if [[ -z "${BUILD_WORKSPACE_DIRECTORY:-}" ]]; then
  echo >&2 "ERROR: update_prebuilts must be run via 'bazel run //pkg:update_prebuilts'"
  exit 1
fi

ZIP_SRC="$1"
PROTOC_VERSION="$2"

if [[ ! -f "$ZIP_SRC" ]]; then
  echo >&2 "ERROR: Could not locate prebuilt archive at $ZIP_SRC"
  exit 1
fi

WORKSPACE_DIR="$(cd "$BUILD_WORKSPACE_DIRECTORY" && (pwd -W 2>/dev/null || pwd))"
PREBUILT_DIR="$WORKSPACE_DIR/.prebuilt_protoc"
mkdir -p "$PREBUILT_DIR"

FILENAME="$(basename "$ZIP_SRC")"
DEST_ZIP="$PREBUILT_DIR/$FILENAME"
rm -f "$DEST_ZIP"
cp "$ZIP_SRC" "$DEST_ZIP"

if command -v sha256sum >/dev/null 2>&1; then
  read -r SHA _ < <(sha256sum < "$DEST_ZIP")
else
  read -r SHA _ < <(shasum -a 256 < "$DEST_ZIP")
fi

FILE_URL="file:///${PREBUILT_DIR#/}/${FILENAME}"
RELEASE_VERSION="v${PROTOC_VERSION//-rc-/-rc}"

OUT_FILE="$WORKSPACE_DIR/bazel/private/oss/toolchains/prebuilt/tool_integrity.bzl"
cat > "$OUT_FILE" <<EOF
"""Locally generated prebuilt protoc integrity manifest."""

RELEASE_VERSION = "${RELEASE_VERSION}"
RELEASED_BINARY_INTEGRITY = {
    "${FILENAME}": {
        "url": "${FILE_URL}",
        "sha256": "${SHA}",
    },
}
EOF

echo "Updated $OUT_FILE with $FILENAME ($SHA)"
