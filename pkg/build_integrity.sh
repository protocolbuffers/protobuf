#!/usr/bin/env bash
#
# Digests all *.zip archives in <zip_dir> and writes tool_integrity.bzl to
# <out_file>.
#
# Usage:
#   build_integrity.sh <zip_dir> <version_tag> <out_file> [url_prefix]
#
# By default, emits the dictionary format mapping each filename to
# {"url": ..., "sha256": ...}. If LEGACY_FORMAT=1 is set in the environment,
# emits the legacy format mapping each filename directly to its "<sha256>"
# string.

set -euo pipefail

ZIP_DIR="$1"
VERSION_TAG="$2"
OUT_FILE="$3"
URL_PREFIX="${4:-https://github.com/protocolbuffers/protobuf/releases/download/$VERSION_TAG}"
LEGACY_FORMAT="${LEGACY_FORMAT:-0}"

# Determine sha256sum binary location:
# 1. On Rapid, it is located at msv/packages/coreutils/usr/bin/sha256sum relative to the script.
# 2. On Linux/Windows Git Bash, sha256sum is on PATH.
# 3. On macOS, fall back to shasum -a 256.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHA256SUM=()
if [[ -x "$SCRIPT_DIR/msv/packages/coreutils/usr/bin/sha256sum" ]]; then
  SHA256SUM=("$SCRIPT_DIR/msv/packages/coreutils/usr/bin/sha256sum")
elif command -v sha256sum >/dev/null 2>&1; then
  SHA256SUM=("sha256sum")
else
  SHA256SUM=("shasum" "-a" "256")
fi

echo '"""Pre-computed during release"""' > "$OUT_FILE"
echo "RELEASE_VERSION = \"$VERSION_TAG\"" >> "$OUT_FILE"
echo "RELEASED_BINARY_INTEGRITY = {" >> "$OUT_FILE"

for f in "$ZIP_DIR"/*.zip; do
  printf "filename: \"$f\" \n"
  [[ -f "$f" ]] || continue
  filename=$(basename "$f")
  read -r sha _ < <("${SHA256SUM[@]}" "$f")
  if (( LEGACY_FORMAT )); then
    echo "  \"$filename\": \"$sha\"," >> "$OUT_FILE"
  else
    echo "  \"$filename\": {" >> "$OUT_FILE"
    echo "    \"url\": \"$URL_PREFIX/$filename\"," >> "$OUT_FILE"
    echo "    \"sha256\": \"$sha\"," >> "$OUT_FILE"
    echo "  }," >> "$OUT_FILE"
  fi
done
echo "}" >> "$OUT_FILE"
