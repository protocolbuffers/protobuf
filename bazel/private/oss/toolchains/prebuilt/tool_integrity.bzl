"""Release binary integrity hashes.

This file contents are entirely replaced during release publishing, by .github/workflows/release_prep.sh
so that the integrity of the prebuilt tools is included in the release artifact.

The checked in content is only here to allow load() statements in the sources to resolve, and permit local testing.
"""

# An arbitrary version of protobuf that includes pre-built binaries.
# See /examples/example_without_cc_toolchain which uses this for testing.
# TODO: add some automation to update this version occasionally.
_TEST_VERSION = "v33.0"
_TEST_SHAS = dict()

# Add a couple platforms which are commonly used for testing.
_TEST_SHAS["protoc-33.0-linux-x86_64.zip"] = "d99c011b799e9e412064244f0be417e5d76c9b6ace13a2ac735330fa7d57ad8f"
_TEST_SHAS["protoc-33.0-osx-aarch_64.zip"] = "3cf55dd47118bd2efda9cd26b74f8bbbfcf5beb1bf606bc56ad4c001b543f6d3"
_TEST_SHAS["protoc-33.0-win64.zip"] = "3742cd49c8b6bd78b6760540367eb0ff62fa70a1032e15dafe131bfaf296986a"

RELEASE_VERSION = _TEST_VERSION
RELEASED_BINARY_INTEGRITY = _TEST_SHAS
