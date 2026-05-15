"""Load dependencies needed to compile the protobuf library as a 3rd-party consumer.

The consumers should use the following WORKSPACE snippet, which loads dependencies
and sets up the repositories protobuf needs:

```
http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-VERSION",
    sha256 = ...,
    url = ...,
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

load("@rules_java//java:rules_java_deps.bzl", "rules_java_dependencies")

rules_java_dependencies()

load("@rules_java//java:repositories.bzl", "rules_java_toolchains")

rules_java_toolchains()

load("@rules_python//python:repositories.bzl", "py_repositories")

py_repositories()
```
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//bazel/private/oss:proto_bazel_features.bzl", "proto_bazel_features")  # buildifier: disable=bzl-visibility
load("//python/dist:python_downloads.bzl", "python_nuget_package", "python_source_archive")
load("//python/dist:system_python.bzl", "system_python")

PROTOBUF_MAVEN_ARTIFACTS = [
    "com.google.code.findbugs:jsr305:3.0.2",
    "com.google.code.gson:gson:2.8.9",
    "com.google.errorprone:error_prone_annotations:2.5.1",
    "com.google.j2objc:j2objc-annotations:2.8",
    "com.google.guava:guava:32.0.1-jre",
]

def _github_archive(repo, commit, **kwargs):
    repo_name = repo.split("/")[-1]
    http_archive(
        urls = [repo + "/archive/" + commit + ".zip"],
        strip_prefix = repo_name + "-" + commit,
        **kwargs
    )

def protobuf_deps():
    """Loads common dependencies needed to compile the protobuf library."""

    if not native.existing_rule("bazel_features"):
        http_archive(
            name = "bazel_features",
            sha256 = "c41853e3b636c533b86bf5ab4658064e6cc9db0a3bce52cbff0629e094344ca9",
            strip_prefix = "bazel_features-1.33.0",
            url = "https://github.com/bazel-contrib/bazel_features/releases/download/v1.33.0/bazel_features-v1.33.0.tar.gz",
        )

    if not native.existing_rule("bazel_skylib"):
        http_archive(
            name = "bazel_skylib",
            sha256 = "3b5b49006181f5f8ff626ef8ddceaa95e9bb8ad294f7b5d7b11ea9f7ddaf8c59",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.9.0/bazel-skylib-1.9.0.tar.gz",
                "https://github.com/bazelbuild/bazel-skylib/releases/download/1.9.0/bazel-skylib-1.9.0.tar.gz",
            ],
        )

    if not native.existing_rule("abseil-cpp"):
        _github_archive(
            name = "abseil-cpp",
            repo = "https://github.com/abseil/abseil-cpp",
            commit = "76bb24329e8bf5f39704eb10d21b9a80befa7c81",  # Abseil LTS 20250512.1
            integrity = "sha256-jF3/tZRlrthY/Y+cEgf1ljqPmtqNOcwVh392LHtERWA=",
        )

    if not native.existing_rule("zlib"):
        http_archive(
            name = "zlib",
            build_file = Label("//third_party:zlib.BUILD"),
            sha256 = "38ef96b8dfe510d42707d9c781877914792541133e1870841463bfa73f883e32",
            strip_prefix = "zlib-1.3.1",
            urls = [
                "https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.xz",
                "https://zlib.net/zlib-1.3.1.tar.xz",
            ],
        )

    if not native.existing_rule("jsoncpp"):
        _github_archive(
            name = "jsoncpp",
            repo = "https://github.com/open-source-parsers/jsoncpp",
            commit = "89e2973c754a9c02a49974d839779b151e95afd6",  # 1.9.6
            sha256 = "02f0804596c1e18c064d890ac9497fa17d585e822fcacf07ff8a8aa0b344a7bd",
            build_file = Label("//third_party:jsoncpp.BUILD"),
        )

    if not native.existing_rule("rules_cc"):
        http_archive(
            name = "rules_cc",
            urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.16/rules_cc-0.0.16.tar.gz"],
            sha256 = "bbf1ae2f83305b7053b11e4467d317a7ba3517a12cef608543c1b1c5bf48a4df",
            strip_prefix = "rules_cc-0.0.16",
        )

    if not native.existing_rule("rules_java"):
        http_archive(
            name = "rules_java",
            urls = [
                "https://github.com/bazelbuild/rules_java/releases/download/8.6.1/rules_java-8.6.1.tar.gz",
            ],
            sha256 = "c5bc17e17bb62290b1fd8fdd847a2396d3459f337a7e07da7769b869b488ec26",
        )

    if not native.existing_rule("rules_shell"):
        http_archive(
            name = "rules_shell",
            sha256 = "410e8ff32e018b9efd2743507e7595c26e2628567c42224411ff533b57d27c28",
            strip_prefix = "rules_shell-0.2.0",
            url = "https://github.com/bazelbuild/rules_shell/releases/download/v0.2.0/rules_shell-v0.2.0.tar.gz",
        )

    if not native.existing_rule("proto_bazel_features"):
        proto_bazel_features(name = "proto_bazel_features")

    if not native.existing_rule("rules_python"):
        http_archive(
            name = "rules_python",
            sha256 = "fa7dd2c6b7d63b3585028dd8a90a6cf9db83c33b250959c2ee7b583a6c130e12",
            strip_prefix = "rules_python-1.6.0",
            url = "https://github.com/bazel-contrib/rules_python/releases/download/1.6.0/rules_python-1.6.0.tar.gz",
        )

    if not native.existing_rule("system_python"):
        system_python(
            name = "system_python",
            minimum_python_version = "3.10",
        )

    if not native.existing_rule("rules_jvm_external"):
        http_archive(
            name = "rules_jvm_external",
            strip_prefix = "rules_jvm_external-6.7",
            sha256 = "a1e351607f04fed296ba33c4977d3fe2a615ed50df7896676b67aac993c53c18",
            url = "https://github.com/bazelbuild/rules_jvm_external/releases/download/6.7/rules_jvm_external-6.7.tar.gz",
        )

    if not native.existing_rule("rules_pkg"):
        http_archive(
            name = "rules_pkg",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/rules_pkg/releases/download/1.0.1/rules_pkg-1.0.1.tar.gz",
                "https://github.com/bazelbuild/rules_pkg/releases/download/1.0.1/rules_pkg-1.0.1.tar.gz",
            ],
            sha256 = "d20c951960ed77cb7b341c2a59488534e494d5ad1d30c4818c736d57772a9fef",
        )

    if not native.existing_rule("build_bazel_apple_support"):
        http_archive(
            name = "build_bazel_apple_support",
            sha256 = "1ae6fcf983cff3edab717636f91ad0efff2e5ba75607fdddddfd6ad0dbdfaf10",
            url = "https://github.com/bazelbuild/apple_support/releases/download/1.24.5/apple_support.1.24.5.tar.gz",
        )

    if not native.existing_rule("rules_kotlin"):
        http_archive(
            name = "rules_kotlin",
            sha256 = "3b772976fec7bdcda1d84b9d39b176589424c047eb2175bed09aac630e50af43",
            url = "https://github.com/bazelbuild/rules_kotlin/releases/download/v1.9.6/rules_kotlin-v1.9.6.tar.gz",
        )

    if not native.existing_rule("rules_license"):
        http_archive(
            name = "rules_license",
            urls = [
                "https://mirror.bazel.build/github.com/bazelbuild/rules_license/releases/download/1.0.0/rules_license-1.0.0.tar.gz",
                "https://github.com/bazelbuild/rules_license/releases/download/1.0.0/rules_license-1.0.0.tar.gz",
            ],
            sha256 = "26d4021f6898e23b82ef953078389dd49ac2b5618ac564ade4ef87cced147b38",
        )

    # Python Downloads
    python_source_archive(
        version = "3.10.0",
        sha256 = "c4e0cbad57c90690cb813fb4663ef670b4d0f587d8171e2c42bd4c9245bd2758",
    )
    python_nuget_package(
        version = "3.10.0",
        cpu = "i686",
        sha256 = "e115e102eb90ce160ab0ef7506b750a8d7ecc385bde0a496f02a54337a8bc333",
    )
    python_nuget_package(
        version = "3.10.0",
        cpu = "x86-64",
        sha256 = "4474c83c25625d93e772e926f95f4cd398a0abbb52793625fa30f39af3d2cc00",
    )
    native.register_toolchains("//bazel/private/oss/toolchains:all")
