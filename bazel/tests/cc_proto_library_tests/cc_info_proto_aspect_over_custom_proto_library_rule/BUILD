load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS", "provides_proto_info")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "lib1",
    srcs = ["a.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

provides_proto_info(
    name = "provider",
    proto_dep = ":lib1",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "consumer",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":provider"],
)

cc_proto_library(
    name = "consumer_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":consumer"],
)

bzl_test_suite(
    name = "tests",
    tests = TESTS,
)
