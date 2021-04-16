load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def upb_deps():
    maybe(
        git_repository,
        name = "com_google_absl",
        commit = "998805a4c79d5d7a771f7e5a8ee3cbbbcba04f94",
        remote = "https://github.com/abseil/abseil-cpp.git",
        shallow_since = "1583355457 -0500",
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
        sha256 = "e5470e92a18aa51830db99a4d9c492cc613761d5bdb7131c04bd92b9834380f6",
        strip_prefix = "rules_python-4b84ad270387a7c439ebdccfd530e2339601ef27",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_python/archive/4b84ad270387a7c439ebdccfd530e2339601ef27.tar.gz",
            "https://github.com/bazelbuild/rules_python/archive/4b84ad270387a7c439ebdccfd530e2339601ef27.tar.gz",
        ],
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
