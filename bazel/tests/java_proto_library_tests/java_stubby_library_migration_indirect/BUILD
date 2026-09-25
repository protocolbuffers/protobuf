load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_proto_library_tests/test_utils.bzl", "java_info_subject_factory")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "inner_proto",
    srcs = ["inner.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "outer_proto",
    srcs = ["outer.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":inner_proto"],
)

java_proto_library(
    name = "outer_java_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":outer_proto"],
)

bzl_test_suite(
    name = "tests",
    provider_subject_factories = [java_info_subject_factory],
    tests = TESTS,
)
