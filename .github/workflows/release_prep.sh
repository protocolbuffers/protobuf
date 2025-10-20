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
INTEGRITY_FILE=${PREFIX}/bazel/private/prebuilt_tool_integrity.bzl

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

mkdir -p ${PREFIX}/bazel/private
cat >${INTEGRITY_FILE} <<EOF
"Generated during release by release_prep.sh"

RELEASE_VERSION="${TAG}"
RELEASED_BINARY_INTEGRITY = $(
curl -s https://api.github.com/repos/protocolbuffers/protobuf/releases/tags/${TAG} \
  | jq -f <(echo "$filter_releases")
)
EOF

# Append that generated file back into the archive
tar --file $ARCHIVE_TMP --append ${INTEGRITY_FILE}

# END patch up the archive
############

gzip < $ARCHIVE_TMP > $ARCHIVE
SHA=$(shasum -a 256 $ARCHIVE | awk '{print $1}')
