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
        _github_archive,
        name = "com_google_absl",
        repo = "https://github.com/abseil/abseil-cpp",
        commit = "b971ac5250ea8de900eae9f95e06548d14cd95fe",  # Abseil LTS 20230125.2
        sha256 = "f7c2cb2c5accdcbbbd5c0c59f241a988c0b1da2a3b7134b823c0bd613b1a6880",
    )

    maybe(
        _github_archive,
        name = "com_google_protobuf",
        repo = "https://github.com/protocolbuffers/protobuf",
        commit = "859410bccc59aeeef1c48e34960fe93827767bac",
        sha256 = "64829c7a42556d3b6a3e19908abc98c0bd920823ac7546ca5372903a53a0acea",
        patches = ["@upb//bazel:protobuf.patch"],
    )

    maybe(
        _github_archive,
        name = "utf8_range",
        repo = "https://github.com/protocolbuffers/utf8_range",
        commit = "de0b4a8ff9b5d4c98108bdfe723291a33c52c54f",
        sha256 = "5da960e5e5d92394c809629a03af3c7709d2d3d0ca731dacb3a9fb4bf28f7702",
    )

    maybe(
        http_archive,
        name = "rules_pkg",
        urls = [
            "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/0.7.0/rules_pkg-0.7.0.tar.gz",
            "https://github.com/bazelbuild/rules_pkg/releases/download/0.7.0/rules_pkg-0.7.0.tar.gz",
        ],
        sha256 = "8a298e832762eda1830597d64fe7db58178aa84cd5926d76d5b744d6558941c2",
    )

    maybe(
        _github_archive,
        name = "rules_python",
        repo = "https://github.com/bazelbuild/rules_python",
        commit = "912a5051f51581784fd64094f6bdabf93f6d698f",  # 0.14.0
        sha256 = "a3e4b4ade7c4a52e757b16a16e94d0b2640333062180cba577d81fac087a501d",
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
