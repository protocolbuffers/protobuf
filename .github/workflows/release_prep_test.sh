#!/usr/bin/env bash
set -o errexit -o nounset -o pipefail

# --- begin runfiles.bash initialization ---
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
  # Not running under Bazel; fall back to co-located script
  rlocation() { echo "$(cd "$(dirname "$0")" && pwd)/$(basename "$1")"; }
fi
# --- end runfiles.bash initialization ---

RELEASE_PREP=$(rlocation _main/.github/workflows/release_prep.sh)
TEST_DIR=$(mktemp -d)
trap 'rm -rf "$TEST_DIR"' EXIT

TAG="v99.0"
PREFIX="protobuf-99.0"

##############################
# Fixture: a git repo with a tag and the placeholder integrity file
##############################
cd "$TEST_DIR"
git init -q
git config user.email "test@test.com"
git config user.name "Test"

mkdir -p bazel/private/oss/toolchains/prebuilt
cat > bazel/private/oss/toolchains/prebuilt/tool_integrity.bzl <<'BZL'
"Placeholder"
RELEASE_VERSION = "v0.0.0"
RELEASED_BINARY_INTEGRITY = {}
BZL

echo "compatibility/ export-ignore" > .gitattributes
mkdir -p compatibility
echo "should be excluded" > compatibility/README
echo "# protobuf" > README.md

git add -A
git commit -q -m "initial"
git tag "$TAG"

##############################
# Fixture: the script needs GNU tar (--delete/--append); shim it
##############################
TAR=$(command -v gtar || command -v tar)
mkdir -p "$TEST_DIR/.mock_bin"
ln -sf "$TAR" "$TEST_DIR/.mock_bin/tar"

##############################
# Fixture: put jq (from Bazel toolchain) on the PATH
##############################
ln -sf "$(rlocation ${JQ_BIN#"external/"})" "$TEST_DIR/.mock_bin/jq"

##############################
# Fixture: mock curl returning GitHub Releases API response & handling asset downloads
# Handles two cases:
# 1) When -o flag is provided, mocks downloading the integrity file .bzl,
#    writing to path specified by the flag
# 2) Otherwise, mocks retrieving the GitHub Releases API JSON, writing to stdout
##############################
cat > "$TEST_DIR/.mock_bin/curl" <<'MOCK'
#!/usr/bin/env bash
outfile=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    -o)
      outfile="$2"
      shift 2
      ;;
    *)
      shift
      ;;
  esac
done

if [[ -n "$outfile" ]]; then
  osx_hash="bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  if [[ "${MOCK_INTEGRITY_MISMATCH:-0}" == "1" ]]; then
    osx_hash="dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
  elif [[ "${MOCK_INVALID_HASH:-0}" == "1" ]]; then
    osx_hash="not-a-sha256"
  fi
  cat <<BZL > "$outfile"
RELEASE_VERSION="v99.0"
RELEASED_BINARY_INTEGRITY = {
    "protoc-99.0-linux-x86_64.zip": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "protoc-99.0-osx-aarch_64.zip": "$osx_hash",
    "protoc-99.0-win64.zip": "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
}
BZL
else
  osx_asset='    {
      "name": "protoc-99.0-osx-aarch_64.zip",
      "digest": "sha256:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    },'
  if [[ "${MOCK_MISSING_ASSET:-0}" == "1" ]]; then
    osx_asset=""
  fi
  cat <<JSON
{
  "assets": [
    {
      "name": "protoc-99.0-linux-x86_64.zip",
      "digest": "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    },
$osx_asset
    {
      "name": "protoc-99.0-win64.zip",
      "digest": "sha256:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    },
    {
      "name": "tool_integrity.bzl",
      "browser_download_url": "https://github.com/protocolbuffers/protobuf/releases/download/v99.0/tool_integrity.bzl"
    }
  ]
}
JSON
fi
MOCK
chmod +x "$TEST_DIR/.mock_bin/curl"
export PATH="$TEST_DIR/.mock_bin:$PATH"

##############################
# Run the script under test
##############################
bash "$RELEASE_PREP" "$TAG"

##############################
# Assertions
##############################
ARCHIVE="$PREFIX.bazel.tar.gz"
FAILURES=0

fail() {
  echo "FAIL: $1"
  FAILURES=$((FAILURES + 1))
}

pass() {
  echo "PASS: $1"
}

assert_file_exists() {
  [[ -f "$1" ]] && pass "$2" || fail "$2"
}

assert_file_absent() {
  [[ ! -e "$1" ]] && pass "$2" || fail "$2"
}

assert_contains() {
  if echo "$3" | grep -qF -- "$2"; then
    pass "$1"
  else
    fail "$1 — expected to find: $2"
  fi
}

# 1. Archive is produced with the expected name
assert_file_exists "$ARCHIVE" "archive file $ARCHIVE exists"

# 2. Archive is gzip-compressed
if file "$ARCHIVE" | grep -q gzip; then
  pass "archive is gzip"
else
  fail "archive is gzip"
fi

# 3. Extract and inspect
EXTRACT_DIR=$(mktemp -d)
tar xzf "$ARCHIVE" -C "$EXTRACT_DIR"

# 4. Patched tool_integrity.bzl is present
INTEGRITY="$EXTRACT_DIR/$PREFIX/bazel/private/oss/toolchains/prebuilt/tool_integrity.bzl"
assert_file_exists "$INTEGRITY" "tool_integrity.bzl present in archive"

CONTENT=$(cat "$INTEGRITY")

# 5. RELEASE_VERSION matches the tag
assert_contains "RELEASE_VERSION matches tag" "RELEASE_VERSION=\"$TAG\"" "$CONTENT"

# 6. RELEASED_BINARY_INTEGRITY is populated from mock curl
assert_contains "integrity map present" "RELEASED_BINARY_INTEGRITY =" "$CONTENT"
assert_contains "linux hash" \
  '"protoc-99.0-linux-x86_64.zip": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"' \
  "$CONTENT"
assert_contains "osx hash" \
  '"protoc-99.0-osx-aarch_64.zip": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"' \
  "$CONTENT"
assert_contains "win hash" \
  '"protoc-99.0-win64.zip": "cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"' \
  "$CONTENT"

# 7. Original placeholder content is NOT in the patched file
if echo "$CONTENT" | grep -qF "v0.0.0"; then
  fail "placeholder content should be replaced"
else
  pass "placeholder content replaced"
fi

# 8. compatibility/ excluded from archive (via .gitattributes export-ignore)
assert_file_absent "$EXTRACT_DIR/$PREFIX/compatibility" "compatibility/ excluded from archive"

# 9. Other repo files are included
assert_file_exists "$EXTRACT_DIR/$PREFIX/README.md" "README.md included in archive"

# 10. All archive entries live under the expected prefix directory
BAD_ENTRIES=$(tar tzf "$ARCHIVE" | grep -cv "^${PREFIX}/") || true
if [[ "$BAD_ENTRIES" -eq 0 ]]; then
  pass "all entries under $PREFIX/ prefix"
else
  fail "found $BAD_ENTRIES entries outside $PREFIX/ prefix"
fi

rm -rf "$EXTRACT_DIR"

# 11. A stale pre-computed hash prevents release archive creation
rm -f "$ARCHIVE"
if MISMATCH_OUTPUT=$(MOCK_INTEGRITY_MISMATCH=1 bash "$RELEASE_PREP" "$TAG" 2>&1); then
  fail "mismatched release binary hash should fail"
else
  assert_contains "mismatched release binary hash fails" \
    "Release binary integrity validation failed" "$MISMATCH_OUTPUT"
  assert_contains "mismatch identifies the affected asset" \
    "hash mismatch for protoc-99.0-osx-aarch_64.zip" "$MISMATCH_OUTPUT"
fi
assert_file_absent "$ARCHIVE" "mismatched hash does not produce an archive"

# 12. A missing release asset also prevents release archive creation
if MISSING_OUTPUT=$(MOCK_MISSING_ASSET=1 bash "$RELEASE_PREP" "$TAG" 2>&1); then
  fail "missing release binary should fail"
else
  assert_contains "missing release binary fails" \
    "Release binary integrity validation failed" "$MISSING_OUTPUT"
  assert_contains "missing binary identifies the affected asset" \
    "missing release asset: protoc-99.0-osx-aarch_64.zip" "$MISSING_OUTPUT"
fi
assert_file_absent "$ARCHIVE" "missing binary does not produce an archive"

# 13. A malformed pre-computed hash also prevents release archive creation
if INVALID_OUTPUT=$(MOCK_INVALID_HASH=1 bash "$RELEASE_PREP" "$TAG" 2>&1); then
  fail "malformed release binary hash should fail"
else
  assert_contains "malformed release binary hash fails" \
    "Release binary integrity validation failed" "$INVALID_OUTPUT"
  assert_contains "malformed hash identifies the affected asset" \
    "invalid hash for protoc-99.0-osx-aarch_64.zip: not-a-sha256" "$INVALID_OUTPUT"
fi
assert_file_absent "$ARCHIVE" "malformed hash does not produce an archive"

##############################
echo
if [[ "$FAILURES" -gt 0 ]]; then
  echo "$FAILURES test(s) FAILED"
  exit 1
else
  echo "All tests passed"
fi
