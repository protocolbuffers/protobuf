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

# Use jq to translate GitHub Releases json into a Starlark object
filter_releases=$(cat <<'EOF'
# Read the file assets already present on the release
reduce .assets[] as $a (
  # Start with an empty dictionary, and for each asset, add
  {}; . + {
    # The format required in starlark, i.e. "release-name": "deadbeef123"
    ($a.name): ($a.digest | sub("^sha256:"; ""))
  }
)
EOF
)

mkdir -p "$(dirname "$INTEGRITY_FILE")"

# Fetch release payload once
RELEASE_API_URL="https://api.github.com/repos/protocolbuffers/protobuf/releases/tags/${TAG}"
RELEASE_JSON=$(curl -sSL "$RELEASE_API_URL")

# Extract the download URL for tool_integrity.bzl if it exists
INTEGRITY_ASSET_URL=$(echo "$RELEASE_JSON" | jq -r '.assets[] | select(.name=="tool_integrity.bzl") | .browser_download_url')

# Check if the asset was found (jq emits "null" or empty if missing)
if [[ -n "$INTEGRITY_ASSET_URL" && "$INTEGRITY_ASSET_URL" != "null" ]]; then
  curl -sSL -o "${INTEGRITY_FILE}" "$INTEGRITY_ASSET_URL"
else
  cat >"${INTEGRITY_FILE}" <<EOF
"""Generated during release by release_prep.sh"""

RELEASE_VERSION="${TAG}"
RELEASED_BINARY_INTEGRITY = $(
  echo "$RELEASE_JSON" | jq -f <(echo "$filter_releases")
)
EOF
fi

# Append that generated file back into the archive
tar --file $ARCHIVE_TMP --append ${INTEGRITY_FILE}

# END patch up the archive
############

gzip < $ARCHIVE_TMP > $ARCHIVE
SHA=$(shasum -a 256 $ARCHIVE | awk '{print $1}')
