
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("//bazel:repository_defs.bzl", "bazel_version_repository")

def upb_deps():
    bazel_version_repository(
        name = "upb_bazel_version",
    )

    git_repository(
        name = "absl",
        commit = "070f6e47b33a2909d039e620c873204f78809492",
        remote = "https://github.com/abseil/abseil-cpp.git",
        shallow_since = "1541627663 -0500",
    )

    git_repository(
        name = "com_google_protobuf",
        remote = "https://github.com/protocolbuffers/protobuf.git",
        commit = "d41002663fd04325ead28439dfd5ce2822b0d6fb",
    )

    http_archive(
        name = "bazel_skylib",
        strip_prefix = "bazel-skylib-master",
        urls = ["https://github.com/bazelbuild/bazel-skylib/archive/master.tar.gz"],
    )

    http_archive(
        name = "zlib",
        build_file = "@com_google_protobuf//:third_party/zlib.BUILD",
        sha256 = "629380c90a77b964d896ed37163f5c3a34f6e6d897311f1df2a7016355c45eff",
        strip_prefix = "zlib-1.2.11",
        url = "https://github.com/madler/zlib/archive/v1.2.11.tar.gz",
    )
