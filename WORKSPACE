workspace(name = "com_google_protobuf")

local_repository(
    name = "com_google_protobuf_examples",
    path = "examples",
)

local_repository(
    name = "submodule_gmock",
    path = "third_party/googletest",
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

bind(
    name = "gtest",
    actual = "@submodule_gmock//:gtest",
)

bind(
    name = "gtest_main",
    actual = "@submodule_gmock//:gtest_main",
)

jvm_maven_import_external(
    name = "guava_maven",
    artifact = "com.google.guava:guava:18.0",
    artifact_sha256 = "d664fbfc03d2e5ce9cab2a44fb01f1d0bf9dfebeccc1a473b1f9ea31f79f6f99",
    server_urls = [
        "https://jcenter.bintray.com/",
        "https://repo1.maven.org/maven2",
    ],
)

bind(
    name = "guava",
    actual = "@guava_maven//jar",
)

jvm_maven_import_external(
    name = "gson_maven",
    artifact = "com.google.code.gson:gson:2.7",
    artifact_sha256 = "2d43eb5ea9e133d2ee2405cc14f5ee08951b8361302fdd93494a3a997b508d32",
    server_urls = [
        "https://jcenter.bintray.com/",
        "https://repo1.maven.org/maven2",
    ],
)

bind(
    name = "gson",
    actual = "@gson_maven//jar",
)

jvm_maven_import_external(
    name = "error_prone_annotations_maven",
    artifact = "com.google.errorprone:error_prone_annotations:2.3.2",
    artifact_sha256 = "357cd6cfb067c969226c442451502aee13800a24e950fdfde77bcdb4565a668d",
    server_urls = [
        "https://jcenter.bintray.com/",
        "https://repo1.maven.org/maven2",
    ],
)

bind(
    name = "error_prone_annotations",
    actual = "@error_prone_annotations_maven//jar",
)

# For `cc_proto_blacklist_test`.
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()
