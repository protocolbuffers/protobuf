load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

filegroup(
    name = "rule_src_filegroup",
    srcs = ["rule_src.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "rule_src_proto",
    srcs = [":rule_src_filegroup"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

cc_proto_library(
    name = "rule_src_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":rule_src_proto"],
)

cc_binary(
    name = "cctest_rule_src",
    srcs = ["cctest_rule_src.cc"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":rule_src_cc_proto"],
)

bzl_test_suite(
    name = "tests",
    tests = TESTS,
)
