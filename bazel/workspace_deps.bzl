load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("//bazel:python_downloads.bzl", "python_nuget_package", "python_source_archive")
load("//bazel:system_python.bzl", "system_python")

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
        url = "https://github.com/abseil/abseil-cpp/archive/b9b925341f9e90f5e7aa0cf23f036c29c7e454eb.zip",
        strip_prefix = "abseil-cpp-b9b925341f9e90f5e7aa0cf23f036c29c7e454eb",
        sha256 = "bb2a0b57c92b6666e8acb00f4cbbfce6ddb87e83625fb851b0e78db581340617",
    )

    maybe(
        _github_archive,
        name = "com_google_protobuf",
        repo = "https://github.com/protocolbuffers/protobuf",
        commit = "14803e6f63d4785ecd95adeeae3ac42a728b3857",
        patches = ["//bazel:protobuf.patch"],
    )

    rules_python_version = "740825b7f74930c62f44af95c9a4c1bd428d2c53"  # Latest @ 2021-06-23

    maybe(
        http_archive,
        name = "rules_python",
        strip_prefix = "rules_python-{}".format(rules_python_version),
        url = "https://github.com/bazelbuild/rules_python/archive/{}.zip".format(rules_python_version),
        sha256 = "09a3c4791c61b62c2cbc5b2cbea4ccc32487b38c7a2cc8f87a794d7a659cc742",
    )

    maybe(
        http_archive,
        name = "bazel_skylib",
        strip_prefix = "bazel-skylib-main",
        urls = ["https://github.com/bazelbuild/bazel-skylib/archive/main.tar.gz"],
    )

    system_python(
        name = "system_python",
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
