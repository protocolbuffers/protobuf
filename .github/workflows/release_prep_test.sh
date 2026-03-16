#!/usr/bin/env bash
set -o errexit -o nounset -o pipefail

exit 1
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
# Fixture: mock curl returning a GitHub Releases API response
##############################
cat > "$TEST_DIR/.mock_bin/curl" <<'MOCK'
#!/usr/bin/env bash
cat <<'JSON'
{
  "assets": [
    {
      "name": "protoc-99.0-linux-x86_64.zip",
      "digest": "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    },
    {
      "name": "protoc-99.0-osx-aarch_64.zip",
      "digest": "sha256:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    },
    {
      "name": "protoc-99.0-win64.zip",
      "digest": "sha256:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
    }
  ]
}
JSON
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

##############################
echo
if [[ "$FAILURES" -gt 0 ]]; then
  echo "$FAILURES test(s) FAILED"
  exit 1
else
  echo "All tests passed"
fi
