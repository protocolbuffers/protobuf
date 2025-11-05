"""Release binary integrity hashes.

This file contents are entirely replaced during release publishing, by .github/workflows/release_prep.sh
so that the integrity of the prebuilt tools is included in the release artifact.

The checked in content is only here to allow load() statements in the sources to resolve.
"""

RELEASE_VERSION = "v33.0"

# Fetched from the last release of protobuf, so the example can work
RELEASED_BINARY_INTEGRITY = {
  "MODULE.bazel.intoto.jsonl": "32ee438bf7e3a210a6b7d5e2d272900a2e7f4a1b4f0992fdf0490281771cac3e",
  "protobuf-33.0.tar.gz": "7a796fd9a7947d51e098ebb065d8f8b45ea0ac313ac89cc083456b3005329a1a",
  "protobuf-33.0.zip": "aaddf29b205ed915100a5fd096e8252842b67da9accfb7ba91ec3680ea307e45",
  "protoc-33.0-linux-aarch_64.zip": "4b96bc91f8b54d829b8c3ca2207ff1ceb774843321e4fa5a68502faece584272",
  "protoc-33.0-linux-ppcle_64.zip": "4eb7682900d01e4848fe9b30b9beeffaf9ed2a8d7e8d310c50ed521dbb33411c",
  "protoc-33.0-linux-s390_64.zip": "96ee21d761e93bbfa7095ed14e769446c8d9790fecfbd7d6962e858350e0da95",
  "protoc-33.0-linux-x86_32.zip": "49edaf078e48d4f45b17be31076ac7dbf64474cd7f1ee3b2cac0938bf0f778f3",
  "protoc-33.0-linux-x86_64.zip": "d99c011b799e9e412064244f0be417e5d76c9b6ace13a2ac735330fa7d57ad8f",
  "protoc-33.0-osx-aarch_64.zip": "3cf55dd47118bd2efda9cd26b74f8bbbfcf5beb1bf606bc56ad4c001b543f6d3",
  "protoc-33.0-osx-universal_binary.zip": "88c0a52f048827d6892cd3403e3ae4181208ab261f93428c86d1736f536a60ec",
  "protoc-33.0-osx-x86_64.zip": "e4e50a703147a92d1a5a2d3a34c9e41717f67ade67d4be72b9a466eb8f22fe87",
  "protoc-33.0-win32.zip": "3941cc8aeb0e8f59f2143b65f594088f726bb857550dabae5d0dee3bf1392dd1",
  "protoc-33.0-win64.zip": "3742cd49c8b6bd78b6760540367eb0ff62fa70a1032e15dafe131bfaf296986a",
  "source.json.intoto.jsonl": "139f2eabd41a050cfde345589fb565cad4fb4f2d86dc084ad9bca3cf0806b5d6"
}
