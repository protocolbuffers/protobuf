"List of published platforms on protobuf GitHub releases"

# Keys are chosen to match the filenames published on protocolbuffers/protobuf releases
# NB: keys in this list are nearly identical to /toolchain/BUILD.bazel#TOOLCHAINS
# Perhaps we should share code.
PROTOBUF_PLATFORMS = {
    # "k8", # this is in /toolchain/BUILD.bazel but not a released platform
    # "osx-universal_binary", # this is not in /toolchain/BUILD.bazel
    # but also Bazel will never request it, as we have a darwin binary for each architecture
    "linux-aarch_64": {
        "compatible_with": [
            "@platforms//os:linux",
            "@platforms//cpu:aarch64",
        ],
    },
    "linux-ppcle_64": {
        "compatible_with": [
            "@platforms//os:linux",
            "@platforms//cpu:ppc64le",
        ],
    },
    "linux-s390_64": {
        "compatible_with": [
            "@platforms//os:linux",
            "@platforms//cpu:s390x",
        ],
    },
    "linux-x86_32": {
        "compatible_with": [
            "@platforms//os:linux",
            "@platforms//cpu:x86_32",
        ],
    },
    "linux-x86_64": {
        "compatible_with": [
            "@platforms//os:linux",
            "@platforms//cpu:x86_64",
        ],
    },
    "osx-aarch_64": {
        "compatible_with": [
            "@platforms//os:macos",
            "@platforms//cpu:aarch64",
        ],
    },
    "osx-x86_64": {
        "compatible_with": [
            "@platforms//os:macos",
            "@platforms//cpu:x86_64",
        ],
    },
    "win32": {
        "compatible_with": [
            "@platforms//os:windows",
            "@platforms//cpu:x86_32",
        ],
    },
    "win64": {
        "compatible_with": [
            "@platforms//os:windows",
            "@platforms//cpu:x86_64",
        ],
    },
}
