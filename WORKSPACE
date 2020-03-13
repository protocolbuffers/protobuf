workspace(name = "com_google_protobuf")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

RULES_JVM_EXTERNAL_COMMIT = "0a58d828a5788c3f156435540fe24337ccee8616"
RULES_JVM_EXTERNAL_SHA = "7a4e199606e4c68ee2d8018fc34f31f13a13cedf3f668ca81628aa8be93dc3c3"

http_archive(
    name = "rules_jvm_external",
    sha256 = RULES_JVM_EXTERNAL_SHA,
    strip_prefix = "rules_jvm_external-%s" % RULES_JVM_EXTERNAL_COMMIT,
    urls = [
        "https://github.com/bazelbuild/rules_jvm_external/archive/%s.zip" % RULES_JVM_EXTERNAL_COMMIT,
    ],
)

load("@rules_jvm_external//:defs.bzl", "maven_install")
load("@rules_jvm_external//:specs.bzl", "maven")

VERSIONS = {
    "easymock": "2.2",
    "errorprone": "2.3.2",
    "gson": "2.7",
    "guava": "28.2-android",
    "junit": "4.13",
    "truth": "1.0.1",
}

maven_install(
    name = "maven",
    artifacts = [
        maven.artifact(
            group = "com.google.code.gson",
            artifact = "gson",
            version = VERSIONS["gson"],
        ),
        maven.artifact(
            group = "com.google.errorprone",
            artifact = "error_prone_annotations",
            version = VERSIONS["errorprone"],
        ),
        maven.artifact(
            group = "com.google.guava",
            artifact = "guava",
            version = VERSIONS["guava"],
        ),
        maven.artifact(
            group = "com.google.guava",
            artifact = "guava-testlib",
            version = VERSIONS["guava"],
            # testonly = True,
        ),
        maven.artifact(
            group = "com.google.truth",
            artifact = "truth",
            version = VERSIONS["truth"],
            # testonly = True,
        ),
        maven.artifact(
            group = "junit",
            artifact = "junit",
            version = VERSIONS["junit"],
            # testonly = True,
        ),
        maven.artifact(
            group = "org.easymock",
            artifact = "easymock",
            version = VERSIONS["easymock"],
            # testonly = True,
        ),
        maven.artifact(
            group = "org.easymock",
            artifact = "easymockclassextension",
            version = VERSIONS["easymock"],
            # testonly = True,
        ),
    ],
    fetch_sources = True,
    repositories = [
        "https://repo1.maven.org/maven2",
    ],
    strict_visibility = True,
)

http_archive(
    name = "com_google_googletest",
    sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
    strip_prefix = "googletest-release-1.10.0",
    urls = [
        "https://mirror.bazel.build/github.com/google/googletest/archive/release-1.10.0.tar.gz",
        "https://github.com/google/googletest/archive/release-1.10.0.tar.gz",
    ],
)

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("//:protobuf_deps.bzl", "protobuf_deps")

# Load common dependencies.
protobuf_deps()
load("@bazel_tools//tools/build_defs/repo:jvm.bzl", "jvm_maven_import_external")

bind(
    name = "python_headers",
    actual = "//util/python:python_headers",
)

# TODO(yannic): Remove in 3.13.0.
bind(
    name = "gtest",
    actual = "@com_google_googletest//:gtest",
)

# TODO(yannic): Remove in 3.13.0.
bind(
    name = "gtest_main",
    actual = "@com_google_googletest//:gtest_main",
)

bind(
    name = "guava",
    actual = "@maven//:com_google_guava_guava",
)

bind(
    name = "gson",
    actual = "@maven//:com_google_code_gson_gson",
)

bind(
    name = "error_prone_annotations",
    actual = "@maven//:com_google_errorprone_error_prone_annotations",
)

# For `cc_proto_blacklist_test`.
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()
