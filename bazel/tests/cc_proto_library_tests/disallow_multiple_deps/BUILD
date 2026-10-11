load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "bar_proto",
    srcs = ["bar.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

cc_proto_library(
    name = "multiple_deps_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [
        ":bar_proto",
        ":foo_proto",
    ],
)

cc_proto_library(
    name = "empty_deps_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [],
)

bzl_test_suite(
    name = "tests",
    expect_failure = True,
    tests = TESTS,
)
