load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel:py_proto_library.bzl", "py_proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "b_proto",
    srcs = ["b_proto.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "a_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":b_proto"],
)

py_proto_library(
    name = "a_py_pb2",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":a_proto"],
)

bzl_test_suite(tests = TESTS)
