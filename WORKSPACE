workspace(name = "com_google_protobuf")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "com_google_protobuf_examples",
    path = "examples",
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

http_archive(
    name = "com_github_google_benchmark",
    sha256 = "2a778d821997df7d8646c9c59b8edb9a573a6e04c534c01892a40aa524a7b68c",
    strip_prefix = "benchmark-bf585a2789e30585b4e3ce6baf11ef2750b54677",
    urls = [
        "https://github.com/google/benchmark/archive/bf585a2789e30585b4e3ce6baf11ef2750b54677.zip",
    ],
)

# Load common dependencies.
load("//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

bind(
    name = "python_headers",
    actual = "//util/python:python_headers",
)

load("@rules_jvm_external//:defs.bzl", "maven_install")
maven_install(
    artifacts = [
        "com.google.code.gson:gson:2.8.6",
        "com.google.errorprone:error_prone_annotations:2.3.2",
        "com.google.guava:guava:30.1.1-jre",
        "com.google.truth:truth:1.1.2",
        "junit:junit:4.12",
        "org.easymock:easymock:3.2",
        "org.easymock:easymockclassextension:3.2",
    ],
    repositories = [
        "https://repo1.maven.org/maven2",
        "https://repo.maven.apache.org/maven2",
    ],
    # For updating instructions, see: 
    # https://github.com/bazelbuild/rules_jvm_external#updating-maven_installjson
    maven_install_json = "//:maven_install.json",
)

load("@maven//:defs.bzl", "pinned_maven_install")
pinned_maven_install()

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

bind(
    name = "junit",
    actual = "@maven//:junit_junit",
)

bind(
    name = "easymock",
    actual = "@maven//:org_easymock_easymock",
)

bind(
    name = "easymock_classextension",
    actual = "@maven//:org_easymock_easymockclassextension",
)

bind(
    name = "truth",
    actual = "@maven//:com_google_truth_truth",
)

# For `cc_proto_blacklist_test` and `build_test`.
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()
