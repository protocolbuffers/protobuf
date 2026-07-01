load("@rules_java//java:java_library.bzl", "java_library")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_lite_proto_library.bzl", "java_lite_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_lite_proto_library_tests/test_utils.bzl", "java_info_subject_factory")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "exported1",
    srcs = ["exported1.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "exported2",
    srcs = ["exported2.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "notexported",
    srcs = ["notexported.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "prototop",
    srcs = ["top.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    exports = [
        ":exported1",
        ":exported2",
    ],
    deps = [
        ":exported1",
        ":exported2",
        ":notexported",
    ],
)

java_lite_proto_library(
    name = "java_lite_proto",
    deps = [":prototop"],
)

java_library(
    name = "java_lib",
    srcs = ["Foo.java"],
    deps = [":java_lite_proto"],
)

bzl_test_suite(
    name = "tests",
    provider_subject_factories = [java_info_subject_factory],
    tests = TESTS,
)
