load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS", "cc_proto_library_macro")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "a_p",
    srcs = ["a.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

cc_proto_library_macro(
    name = "a",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":a_p"],
)

bzl_test_suite(
    name = "tests",
    tests = TESTS,
)
