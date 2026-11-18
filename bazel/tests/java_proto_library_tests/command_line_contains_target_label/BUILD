# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:java_proto_library_tests/test_utils.bzl", "JAVA_PROTO_TESTING_ASPECT")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

bzl_test_suite(
    name = "tests",
    testing_aspect = JAVA_PROTO_TESTING_ASPECT,
    tests = TESTS,
)
