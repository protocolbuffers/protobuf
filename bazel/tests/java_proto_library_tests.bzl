# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `java_proto_library`."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests/testdata:my_rule_with_aspect.bzl", "my_rule_with_aspect")

def java_proto_library_test_suite(name):
    util.helper_target(
        proto_library,
        name = "simple2_proto",
        srcs = ["A.proto"],
    )
    test_suite(
        name = name,
        tests = [
            _test_java_info_proto_aspect,
        ],
    )

# Verifies basic usage of `proto_common.compile`.
def _test_java_info_proto_aspect(name):
    util.helper_target(
        my_rule_with_aspect,
        name = "my_rule",
        deps = [":my_java_proto"],
    )
    util.helper_target(
        java_proto_library,
        name = "my_java_proto",
        deps = [":my_proto"],
    )
    util.helper_target(
        proto_library,
        name = "my_proto",
        srcs = ["my.proto"],
    )

    analysis_test(
        name = name,
        target = ":my_rule",
        impl = _test_java_info_proto_aspect_impl,
    )

def _test_java_info_proto_aspect_impl(env, target):
    java_info = env.expect.that_target(target).actual[JavaInfo]
    env.expect.that_depset_of_files(java_info.transitive_source_jars).contains_at_least_predicates(
        [matching.file_basename_equals("my_proto-speed-src.jar")],
    )
    env.expect.that_depset_of_files(java_info.transitive_runtime_jars).contains_at_least_predicates(
        [matching.file_basename_equals("libmy_proto-speed.jar")],
    )
