load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:utils.bzl", "maybe")
load("//bazel:system_python.bzl", "system_python")

def upb_deps():
    maybe(
        http_archive,
        name = "com_google_absl",
        url = "https://github.com/abseil/abseil-cpp/archive/b9b925341f9e90f5e7aa0cf23f036c29c7e454eb.zip",
        strip_prefix = "abseil-cpp-b9b925341f9e90f5e7aa0cf23f036c29c7e454eb",
        sha256 = "bb2a0b57c92b6666e8acb00f4cbbfce6ddb87e83625fb851b0e78db581340617",
    )

    maybe(
        git_repository,
        name = "com_google_protobuf",
        commit = "ec79d0d328c7e6cea15cc27fbeb9b018ca289590",
        remote = "https://github.com/protocolbuffers/protobuf.git",
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
