# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Tests for `cc_proto_library`."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")

def cc_proto_library_test_suite(name):
    """Defines helper targets and a test suite for cc_proto_library tests.

    Args:
        name: The name of the test suite.
    """
    util.helper_target(cc_library, name = name + "_a", srcs = [name + "_a.cc"])
    util.helper_target(proto_library, name = name + "_b", srcs = [name + "_b.proto"])
    util.helper_target(cc_proto_library, name = name + "_bc", deps = [name + "_b"])
    util.helper_target(cc_library, name = name + "_c", srcs = [name + "_c.cc"])

    test_suite(
        name = name,
        tests = [
            _test_link_order_with_mixed_deps,
            _test_link_order_with_mixed_deps_and_intermediate_library,
            _test_link_order_with_mixed_deps_and_linkshared,
        ],
    )

def _test_link_order_with_mixed_deps(name):
    util.helper_target(
        cc_binary,
        name = name + "_foo",
        srcs = ["foo.cc"],
        deps = [
            ":cc_proto_library_tests_a",
            ":cc_proto_library_tests_bc",
            ":cc_proto_library_tests_c",
        ],
        features = ["supports_start_end_lib"],
    )

    analysis_test(
        name = name,
        target = ":" + name + "_foo",
        impl = _test_link_order_with_mixed_deps_impl,
    )

def _test_link_order_with_mixed_deps_impl(env, target):
    action = env.expect.that_target(target).action_named("CppLink")

    # Using file_path_matches because the basename varies between google3, OSS and different
    # platforms.
    action.inputs().contains_at_least_predicates([
        matching.file_path_matches("test_link_order_with_mixed_deps_foo"),
        matching.file_path_matches("cc_proto_library_tests_a"),
        matching.file_path_matches("cc_proto_library_tests_b"),
        # Using matching.any because the basename varies by platform in OSS.
        matching.any(
            matching.file_path_matches("protobuf/message"),
            matching.file_path_matches("protobuf/libprotobuf"),
            matching.file_path_matches("protobuf/protobuf.lib"),
        ),
        matching.file_path_matches("cc_proto_library_tests_c"),
    ]).in_order()

def _test_link_order_with_mixed_deps_and_intermediate_library(name):
    util.helper_target(
        cc_library,
        name = name + "_lib",
        srcs = ["lib.cc"],
        deps = [
            ":cc_proto_library_tests_a",
            ":cc_proto_library_tests_bc",
            ":cc_proto_library_tests_c",
        ],
    )
    util.helper_target(
        cc_binary,
        name = name + "_foo",
        srcs = ["foo.cc"],
        deps = [":" + name + "_lib"],
        features = ["supports_start_end_lib"],
    )
    analysis_test(
        name = name,
        target = ":" + name + "_foo",
        impl = _test_link_order_with_mixed_deps_and_intermediate_library_impl,
    )

def _test_link_order_with_mixed_deps_and_intermediate_library_impl(env, target):
    action = env.expect.that_target(target).action_named("CppLink")

    # Using file_path_matches because the basename varies between google3, OSS and different platforms.
    action.inputs().contains_at_least_predicates([
        matching.file_path_matches("test_link_order_with_mixed_deps_and_intermediate_library_foo"),
        matching.file_path_matches("test_link_order_with_mixed_deps_and_intermediate_library_lib"),
        matching.file_path_matches("cc_proto_library_tests_a"),
        matching.file_path_matches("cc_proto_library_tests_b"),
        # Using matching.any because the basename varies by platform in OSS.
        matching.any(
            matching.file_path_matches("protobuf/message"),
            matching.file_path_matches("protobuf/libprotobuf"),
            matching.file_path_matches("protobuf/protobuf.lib"),
        ),
        matching.file_path_matches("cc_proto_library_tests_c"),
    ]).in_order()

def _test_link_order_with_mixed_deps_and_linkshared(name):
    util.helper_target(
        cc_binary,
        name = name + "_foo.so",
        srcs = ["foo.cc"],
        linkshared = 1,
        deps = [
            ":cc_proto_library_tests_a",
            ":cc_proto_library_tests_bc",
            ":cc_proto_library_tests_c",
        ],
    )

    analysis_test(
        name = name,
        target = ":" + name + "_foo.so",
        impl = _test_link_order_with_mixed_deps_and_linkshared_impl,
    )

def _test_link_order_with_mixed_deps_and_linkshared_impl(env, target):
    action = env.expect.that_target(target).action_named("CppLink")

    action.inputs().contains_at_least_predicates([
        matching.file_path_matches("test_link_order_with_mixed_deps_and_linkshared_foo.so"),
        matching.file_path_matches("cc_proto_library_tests_a"),
        matching.file_path_matches("cc_proto_library_tests_b"),
        # Using matching.any because the basename varies by platform in OSS.
        matching.any(
            matching.file_path_matches("protobuf/message"),
            matching.file_path_matches("protobuf/libprotobuf"),
            matching.file_path_matches("protobuf/protobuf.lib"),
        ),
        matching.file_path_matches("cc_proto_library_tests_c"),
    ]).in_order()
