load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")

def upb_deps():
    maybe(
        git_repository,
        name = "com_google_absl",
        commit = "df3ea785d8c30a9503321a3d35ee7d35808f190d",  # LTS 2020-02-25
        remote = "https://github.com/abseil/abseil-cpp.git",
        shallow_since = "1583355457 -0500",
    )

    maybe(
        git_repository,
        name = "com_google_protobuf",
        remote = "https://github.com/protocolbuffers/protobuf.git",
        commit = "c8f76331abf682c289fa79f05b2ee39cc7bf5a48",  # Need to use Git until proto3 optional is released
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
        strip_prefix = "bazel-skylib-master",
        urls = ["https://github.com/bazelbuild/bazel-skylib/archive/master.tar.gz"],
    )

    maybe(
        http_archive,
        name = "zlib",
        build_file = "@com_google_protobuf//:third_party/zlib.BUILD",
        sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
        strip_prefix = "zlib-1.2.11",
        url = "https://github.com/madler/zlib/archive/v1.2.11.tar.gz",
    )
