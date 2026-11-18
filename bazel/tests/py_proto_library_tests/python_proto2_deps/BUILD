# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_python//python:py_binary.bzl", "py_binary")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel:py_proto_library.bzl", "py_proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

util.empty_file("proto2_deps_bin.py")

proto_library(
    name = "file_proto",
    srcs = ["file.proto"],
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

bzl_test_suite(
    name = "tests",
    tests = TESTS,
)
