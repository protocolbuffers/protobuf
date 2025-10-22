"""Release binary integrity hashes.

This file contents are entirely replaced during release publishing, by .github/workflows/release_prep.sh
so that the integrity of the prebuilt tools is included in the release artifact.

The checked in content is only here to allow load() statements in the sources to resolve.
"""

# Create a mapping for every tool name to the hash of /dev/null
NULLSHA = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
RELEASED_BINARY_INTEGRITY = {
    "-".join([
        "protoc",
        os,
        arch,
    ]): NULLSHA
    for [os, arch] in [
        ("linux", "x86_64"),
        ("osx", "aarch_64"),
        ("win", "x86_64"),
    ]
}
