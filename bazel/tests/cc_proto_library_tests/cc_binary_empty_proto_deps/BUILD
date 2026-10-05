load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

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

proto_library(
    name = "empty_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

cc_proto_library(
    name = "empty_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":empty_proto"],
)

cc_binary(
    name = "cctest_empty_binary",
    srcs = ["cctest_binary.cc"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":empty_cc_proto"],
)

bzl_test_suite(
    name = "tests",
    tests = TESTS,
)
