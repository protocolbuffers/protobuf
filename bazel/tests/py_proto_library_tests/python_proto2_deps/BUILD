load("@rules_python//python:py_binary.bzl", "py_binary")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel:py_proto_library.bzl", "py_proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

util.empty_file("proto2_deps_bin.py")

util.empty_file("file_proto.proto")

proto_library(
    name = "file_proto",
    srcs = ["file_proto.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

py_proto_library(
    name = "file_proto_py_pb2",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":file_proto"],
)

py_binary(
    name = "proto2_deps_bin",
    srcs = ["proto2_deps_bin.py"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":file_proto_py_pb2"],
)

bzl_test_suite(tests = TESTS)
