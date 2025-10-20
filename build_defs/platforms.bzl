"""Stores a map of platforms sets we support and their corresponding constraints."""

PLATFORMS = {
    "osx-x86_64": [
        "@platforms//cpu:x86_64",
        "@platforms//os:osx",
    ],
    "osx-aarch_64": [
        "@platforms//cpu:aarch64",
        "@platforms//os:osx",
    ],
    "linux-aarch_64": [
        "@platforms//cpu:aarch64",
        "@platforms//os:linux",
    ],
    "linux-ppcle_64": [
        "@platforms//cpu:ppc64le",
        "@platforms//os:linux",
    ],
    "linux-s390_64": [
        "@platforms//cpu:s390x",
        "@platforms//os:linux",
    ],
    "linux-x86_32": [
        "@platforms//cpu:x86_32",
        "@platforms//os:linux",
    ],
    "linux-x86_64": [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    "win32": [
        "@platforms//cpu:x86_32",
        "@platforms//os:windows",
    ],
    "win64": [
        "@platforms//cpu:x86_64",
        "@platforms//os:windows",
    ],
    "k8": [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
}
