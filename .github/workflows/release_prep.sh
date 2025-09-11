#!/usr/bin/env bash
# NB: this file must be named release_prep.sh because the attestation generation doesn't trust user control.
# see https://github.com/bazel-contrib/.github/blob/v7.2.3/.github/workflows/release_ruleset.yaml#L33-L45
set -o errexit -o nounset -o pipefail

# Argument provided by reusable workflow caller, see
# https://github.com/bazel-contrib/.github/blob/v7.2.3/.github/workflows/release_ruleset.yaml#L104
TAG=$1
# HACK during debugging, to allow us to fetch older release artifacts.
RELEASE_ARTIFACT_TAG=v32.0
PREFIX="protobuf-${TAG:1}"
ARCHIVE="$PREFIX.tar.gz"
ARCHIVE_TMP=$(mktemp)
INTEGRITY_FILE=${PREFIX}/bazel/private/prebuilt_tool_integrity.bzl

# NB: configuration for 'git archive' is in /.gitattributes
git archive --format=tar --prefix=${PREFIX}/ ${TAG} > $ARCHIVE_TMP
############
# Patch up the archive to have integrity hashes for built binaries that we downloaded in the GHA workflow.
# Now that we've run `git archive` we are free to pollute the working directory.

# Delete the placeholder file
tar --file $ARCHIVE_TMP --delete $INTEGRITY_FILE

mkdir -p ${PREFIX}/bazel/private
cat >${INTEGRITY_FILE} <<EOF
"Generated during release by release_prep.sh"

RELEASED_BINARY_INTEGRITY = $(
curl -s https://api.github.com/repos/protocolbuffers/protobuf/releases/tags/${RELEASE_ARTIFACT_TAG} \
  | jq 'reduce .assets[] as $a ({}; . + { ($a.name): ($a.digest | sub("^sha256:"; "")) })'
)
EOF

# Append that generated file back into the archive
tar --file $ARCHIVE_TMP --append ${INTEGRITY_FILE}

# END patch up the archive
############

gzip < $ARCHIVE_TMP > $ARCHIVE
SHA=$(shasum -a 256 $ARCHIVE | awk '{print $1}')
