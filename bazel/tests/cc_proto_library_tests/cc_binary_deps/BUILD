load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "CC_PROTO_TESTING_ASPECT")
load(":tests.bzl", "TESTS", "TESTS_NO_DYNAMIC_LINKER")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "baz_proto",
    srcs = ["baz.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "foo_proto",
    srcs = [
        "bar.proto",
        "foo.proto",
    ],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":baz_proto"],
)

cc_proto_library(
    name = "foo_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

cc_binary(
    name = "cctest_binary",
    srcs = ["cctest_binary.cc"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_cc_proto"],
)

bzl_test_suite(
    name = "tests",
    testing_aspect = CC_PROTO_TESTING_ASPECT,
    tests = TESTS,
)

bzl_test_suite(
    name = "tests_no_dynamic_linker",
    config_settings = {
        "//command_line_option:features": ["-supports_dynamic_linker"],
    },
    testing_aspect = CC_PROTO_TESTING_ASPECT,
    tests = TESTS_NO_DYNAMIC_LINKER,
)
