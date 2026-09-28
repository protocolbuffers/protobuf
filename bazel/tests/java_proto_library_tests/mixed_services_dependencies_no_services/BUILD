load("@rules_java//java:java_library.bzl", "java_library")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_proto_library_tests/test_utils.bzl", "java_info_subject_factory")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "common_proto",
    srcs = ["common.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "a_proto",
    srcs = ["a.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":common_proto"],
)

proto_library(
    name = "b_proto",
    srcs = ["b.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [
        ":a_proto",
        ":common_proto",
    ],
)

java_proto_library(
    name = "a_java_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":a_proto"],
)

java_proto_library(
    name = "b_java_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":b_proto"],
)

java_library(
    name = "both_java_lib",
    srcs = ["Both.java"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [
        ":a_java_proto",
        ":b_java_proto",
    ],
)

bzl_test_suite(
    name = "tests",
    provider_subject_factories = [java_info_subject_factory],
    tests = TESTS,
)
