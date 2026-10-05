load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS", "cc_equals_proto_library")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "foo_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

cc_proto_library(
    name = "foo_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

cc_equals_proto_library(
    name = "foo_cc_equals_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_cc_proto"],
)

bzl_test_suite(
    name = "tests",
    tests = TESTS,
)
