#!/usr/bin/env bash
#
# Builds a local prebuilt protoc zip archive for the current platform and
# updates bazel/private/oss/toolchains/prebuilt/tool_integrity.bzl in place with
# a file:// URL and SHA-256 hash pointing to it.
#
# Usage:
#   bazel run //pkg:update_prebuilts
#
# Relationship to compute_integrity.sh and build_integrity.sh:
#   During official releases, the Rapid release pipeline builds prebuilt protoc
#   archives across all supported platforms and runs compute_integrity.sh (which
#   delegates to build_integrity.sh) to populate tool_integrity.bzl with
#   https://github.com/.../releases/download/ URLs and SHA-256 hashes for every
#   release archive.
#
#   On development branches (or before a release is published to GitHub),
#   tool_integrity.bzl is checked in with an empty RELEASED_BINARY_INTEGRITY
#   dict. Running this target copies the host platform's prebuilt protoc archive
#   into .prebuilt_protoc/ and invokes build_integrity.sh with a file:// URL
#   prefix so that prebuilt protoc toolchain resolution and authenticity
#   validation can be tested locally and in CI.

set -euo pipefail

if [[ -z "${BUILD_WORKSPACE_DIRECTORY:-}" ]]; then
  echo >&2 "ERROR: update_prebuilts must be run via 'bazel run //pkg:update_prebuilts'"
  exit 1
fi

ZIP_SRC="$1"
PROTOC_VERSION="$2"
BUILD_INTEGRITY="$3"

if [[ ! -f "$ZIP_SRC" ]]; then
  echo >&2 "ERROR: Could not locate prebuilt archive at $ZIP_SRC"
  exit 1
fi

WORKSPACE_DIR="$(cd "$BUILD_WORKSPACE_DIRECTORY" && (pwd -W 2>/dev/null || pwd))"
PREBUILT_DIR="$WORKSPACE_DIR/.prebuilt_protoc"
rm -rf "$PREBUILT_DIR"
mkdir -p "$PREBUILT_DIR"

FILENAME="$(basename "$ZIP_SRC")"
DEST_ZIP="$PREBUILT_DIR/$FILENAME"
cp "$ZIP_SRC" "$DEST_ZIP"

URL_PREFIX="file:///${PREBUILT_DIR#/}"
RELEASE_VERSION="v${PROTOC_VERSION//-rc-/-rc}"
OUT_FILE="$WORKSPACE_DIR/bazel/private/oss/toolchains/prebuilt/tool_integrity.bzl"

bash "$BUILD_INTEGRITY" "$PREBUILT_DIR" "$RELEASE_VERSION" "$OUT_FILE" "$URL_PREFIX"

echo "Updated $OUT_FILE with $FILENAME"
