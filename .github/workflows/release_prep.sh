#!/usr/bin/env bash
# NB: this file must be named release_prep.sh because the attestation generation doesn't trust user control.
# see https://github.com/bazel-contrib/.github/blob/v7.2.3/.github/workflows/release_ruleset.yaml#L33-L45
set -o errexit -o nounset -o pipefail

# Argument provided by reusable workflow caller, see
# https://github.com/bazel-contrib/.github/blob/v7.2.3/.github/workflows/release_ruleset.yaml#L104
TAG=$1
PREFIX="protobuf-${TAG:1}"
ARCHIVE="$PREFIX.bazel.tar.gz"
ARCHIVE_TMP=$(mktemp)
INTEGRITY_FILE=${PREFIX}/bazel/private/oss/toolchains/prebuilt/tool_integrity.bzl

# NB: configuration for 'git archive' is in /.gitattributes
git archive --format=tar --prefix=${PREFIX}/ ${TAG} > $ARCHIVE_TMP
############
# Patch up the archive to have integrity hashes for built binaries that we downloaded in the GHA workflow.
# Now that we've run `git archive` we are free to pollute the working directory.

# Delete the placeholder file
tar --file $ARCHIVE_TMP --delete $INTEGRITY_FILE

mkdir -p "$(dirname "$INTEGRITY_FILE")"

# Fetch release payload once
RELEASE_API_URL="https://api.github.com/repos/protocolbuffers/protobuf/releases/tags/${TAG}"
RELEASE_JSON=$(curl -sSL "$RELEASE_API_URL")

# Extract the download URL for tool_integrity.bzl
INTEGRITY_ASSET_URL=$(echo "$RELEASE_JSON" | jq -r '.assets[] | select(.name=="tool_integrity.bzl") | .browser_download_url')

curl -sSL -o "${INTEGRITY_FILE}" "$INTEGRITY_ASSET_URL"

# Validate the trusted, pre-computed hashes against the uploaded artifacts.
INTEGRITY_ERRORS=$(echo "$RELEASE_JSON" | jq -r --rawfile integrity "${INTEGRITY_FILE}" '
  . as $release
  | [$integrity
     | scan("\\\"(protoc-[^\\\"]+\\.zip)\\\"[[:space:]]*:[[:space:]]*\\\"([^\\\"]*)\\\"")
     | {name: .[0], expected: .[1]}] as $expected
  | if ($expected | length) == 0 then
      "tool_integrity.bzl does not contain any protoc hashes"
    else
      $expected[]
      | . as $entry
      | ($release.assets | map(select(.name == $entry.name)) | first) as $asset
      | if ($entry.expected | test("^[0-9a-f]{64}$") | not) then
          "invalid hash for \($entry.name): \($entry.expected)"
        elif $asset == null then
          "missing release asset: \($entry.name)"
        elif ($asset.digest // "") != ("sha256:" + $entry.expected) then
          "hash mismatch for \($entry.name): expected \($entry.expected), got \($asset.digest // "no digest")"
        else
          empty
        end
    end
')

if [[ -n "$INTEGRITY_ERRORS" ]]; then
  echo "Release binary integrity validation failed:" >&2
  echo "$INTEGRITY_ERRORS" >&2
  exit 1
fi

# Append that generated file back into the archive
tar --file $ARCHIVE_TMP --append ${INTEGRITY_FILE}

# END patch up the archive
############

gzip < $ARCHIVE_TMP > $ARCHIVE
SHA=$(shasum -a 256 $ARCHIVE | awk '{print $1}')
