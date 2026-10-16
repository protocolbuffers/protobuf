load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

cc_proto_library(
    name = "descriptor_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = ["//:descriptor_proto"],
)

bzl_test_suite(
    name = "tests",
    tests = TESTS,
)
