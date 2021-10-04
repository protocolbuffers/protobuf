load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def upb_deps():
    maybe(
        http_archive,
        name = "com_google_absl",
        url = "https://github.com/abseil/abseil-cpp/archive/refs/heads/master.zip",
        strip_prefix = "abseil-cpp-master",
        sha256 = "9da85425cda33e13c619c35473c9653944d0dd9aab1a69ee9b3543cdaed277e5",
    )

    maybe(
        http_archive,
        name = "com_google_protobuf",
        sha256 = "b10bf4e2d1a7586f54e64a5d9e7837e5188fc75ae69e36f215eb01def4f9721b",
        strip_prefix = "protobuf-3.15.3",
        urls = [
            "https://mirror.bazel.build/github.com/protocolbuffers/protobuf/archive/v3.15.3.tar.gz",
            "https://github.com/protocolbuffers/protobuf/archive/v3.15.3.tar.gz",
        ],
    )

    maybe(
        http_archive,
        name = "rules_python",
        url = "https://github.com/bazelbuild/rules_python/releases/download/0.1.0/rules_python-0.1.0.tar.gz",
        sha256 = "b6d46438523a3ec0f3cead544190ee13223a52f6a6765a29eae7b7cc24cc83a0",
    )

    maybe(
        http_archive,
        name = "bazel_skylib",
        strip_prefix = "bazel-skylib-main",
        urls = ["https://github.com/bazelbuild/bazel-skylib/archive/main.tar.gz"],
    )

    maybe(
        http_archive,
        name = "zlib",
        build_file = "@com_google_protobuf//:third_party/zlib.BUILD",
        sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
        strip_prefix = "zlib-1.2.11",
        url = "https://github.com/madler/zlib/archive/v1.2.11.tar.gz",
    )
