load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("//bazel:python_downloads.bzl", "python_nuget_package", "python_source_archive")

def _github_archive(repo, commit, **kwargs):
    repo_name = repo.split("/")[-1]
    http_archive(
        urls = [repo + "/archive/" + commit + ".zip"],
        strip_prefix = repo_name + "-" + commit,
        **kwargs
    )

def upb_deps():
    maybe(
        http_archive,
        name = "com_google_absl",
        url = "https://github.com/abseil/abseil-cpp/archive/e6044634dd7caec2d79a13aecc9e765023768757.tar.gz",
        strip_prefix = "abseil-cpp-e6044634dd7caec2d79a13aecc9e765023768757",
        sha256 = "e7fdfe0bed87702a22c5b73b6b5fe08bedd25f17d617e52df6061b0f47d480b0",
    )

    maybe(
        _github_archive,
        name = "com_google_protobuf",
        repo = "https://github.com/protocolbuffers/protobuf",
        commit = "016ef2393e163cb8a49186adab8e2981476eec37",
        sha256 = "ed91f723ba5db42190ca4a2734d1fdd449ceccf259c1112d3e4da14b322680f5",
        patches = ["@upb//bazel:protobuf.patch"],
    )

    rules_python_version = "0.12.0"  # Latest @ August 31, 2022

    maybe(
        http_archive,
        name = "rules_python",
        strip_prefix = "rules_python-{}".format(rules_python_version),
        url = "https://github.com/bazelbuild/rules_python/archive/refs/tags/{}.tar.gz".format(rules_python_version),
        sha256 = "b593d13bb43c94ce94b483c2858e53a9b811f6f10e1e0eedc61073bd90e58d9c",
    )

    maybe(
        http_archive,
        name = "bazel_skylib",
        strip_prefix = "bazel-skylib-main",
        urls = ["https://github.com/bazelbuild/bazel-skylib/archive/main.tar.gz"],
    )

    #Python Downloads

    python_source_archive(
        name = "python-3.7.0",
        sha256 = "85bb9feb6863e04fb1700b018d9d42d1caac178559ffa453d7e6a436e259fd0d",
    )
    python_nuget_package(
        name = "nuget_python_i686_3.7.0",
        sha256 = "a8bb49fa1ca62ad55430fcafaca1b58015e22943e66b1a87d5e7cef2556c6a54",
    )
    python_nuget_package(
        name = "nuget_python_x86-64_3.7.0",
        sha256 = "66eb796a5bdb1e6787b8f655a1237a6b6964af2115b7627cf4f0032cf068b4b2",
    )
    python_nuget_package(
        name = "nuget_python_i686_3.8.0",
        sha256 = "87a6481f5eef30b42ac12c93f06f73bd0b8692f26313b76a6615d1641c4e7bca",
    )
    python_nuget_package(
        name = "nuget_python_x86-64_3.8.0",
        sha256 = "96c61321ce90dd053c8a04f305a5f6cc6d91350b862db34440e4a4f069b708a0",
    )
    python_nuget_package(
        name = "nuget_python_i686_3.9.0",
        sha256 = "229abecbe49dc08fe5709e0b31e70edfb3b88f23335ebfc2904c44f940fd59b6",
    )
    python_nuget_package(
        name = "nuget_python_x86-64_3.9.0",
        sha256 = "6af58a733e7dfbfcdd50d55788134393d6ffe7ab8270effbf724bdb786558832",
    )
    python_nuget_package(
        name = "nuget_python_i686_3.10.0",
        sha256 = "e115e102eb90ce160ab0ef7506b750a8d7ecc385bde0a496f02a54337a8bc333",
    )
    python_nuget_package(
        name = "nuget_python_x86-64_3.10.0",
        sha256 = "4474c83c25625d93e772e926f95f4cd398a0abbb52793625fa30f39af3d2cc00",
    )
