load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "CC_PROTO_TESTING_ASPECT")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

bzl_test_suite(
    name = "tests",
    attr_values = {
        "target_compatible_with": ["@platforms//os:linux"],
    },
    config_settings = {
        "//command_line_option:collect_code_coverage": True,
        "//command_line_option:features": ["-msan"],
        "//command_line_option:instrumentation_filter": ".",
    },
    testing_aspect = CC_PROTO_TESTING_ASPECT,
    tests = TESTS,
)
