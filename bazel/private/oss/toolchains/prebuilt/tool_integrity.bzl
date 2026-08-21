"""Release binary integrity hashes.

This file contents are entirely replaced during release publishing, by .github/workflows/release_prep.sh
so that the integrity of the prebuilt tools is included in the release artifact.

The checked in content is only here to allow load() statements in the sources to resolve.
"""

load("//:protobuf_version.bzl", "PROTOC_VERSION")

RELEASE_VERSION = "v{}-dev".format(PROTOC_VERSION)
RELEASED_BINARY_INTEGRITY = dict()
