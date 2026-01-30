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
    test_suite(
        name = name,
        tests = [
            _test_link_order_with_mixed_deps,
            _test_link_order_with_mixed_deps_and_intermediate_library,
            _test_proto_library_in_srcs,
        ],
    )

def _test_link_order_with_mixed_deps(name):
    util.helper_target(cc_library, name = name + "_a", srcs = ["a.cc"])
    util.helper_target(proto_library, name = name + "_b", srcs = ["b.proto"])
    util.helper_target(cc_proto_library, name = name + "_bc", deps = [":" + name + "_b"])
    util.helper_target(cc_library, name = name + "_c", srcs = ["c.cc"])
    util.helper_target(
        cc_binary,
        name = name + "_foo",
        srcs = ["foo.cc"],
        deps = [
            ":" + name + "_a",
            ":" + name + "_bc",
            ":" + name + "_c",
        ],
    )

    analysis_test(
        name = name,
        target = ":" + name + "_foo",
        impl = _test_link_order_with_mixed_deps_impl,
    )

def _test_link_order_with_mixed_deps_impl(env, target):
    action = env.expect.that_target(target).action_named("CppLink")
    action.inputs().contains_at_least_predicates([
        matching.file_basename_contains("a.pic.o"),
        matching.file_basename_contains("b.pb.pic.o"),
        matching.file_basename_contains("c.pic.o"),
        matching.file_basename_contains("link.pic.o"),
        matching.file_basename_contains("foo.pic.o"),
    ])

def _test_link_order_with_mixed_deps_and_intermediate_library(name):
    util.helper_target(cc_library, name = name + "_a", srcs = ["a.cc"])
    util.helper_target(proto_library, name = name + "_b", srcs = ["b.proto"])
    util.helper_target(cc_proto_library, name = name + "_bc", deps = [":" + name + "_b"])
    util.helper_target(cc_library, name = name + "_c", srcs = ["c.cc"])
    util.helper_target(
        cc_library,
        name = name + "_lib",
        srcs = ["lib.cc"],
        deps = [
            ":" + name + "_a",
            ":" + name + "_bc",
            ":" + name + "_c",
        ],
    )
    util.helper_target(
        cc_binary,
        name = name + "_foo",
        srcs = ["foo.cc"],
        deps = [":" + name + "_lib"],
    )

    analysis_test(
        name = name,
        target = ":" + name + "_foo",
        impl = _test_link_order_with_mixed_deps_and_intermediate_library_impl,
    )

def _test_link_order_with_mixed_deps_and_intermediate_library_impl(env, target):
    action = env.expect.that_target(target).action_named("CppLink")
    action.inputs().contains_at_least_predicates([
        matching.file_basename_contains("lib.pic.o"),
        matching.file_basename_contains("a.pic.o"),
        matching.file_basename_contains("b.pb.pic.o"),
        matching.file_basename_contains("c.pic.o"),
        matching.file_basename_contains("link.pic.o"),
        matching.file_basename_contains("foo.pic.o"),
    ])

def _test_proto_library_in_srcs(name):
    util.helper_target(proto_library, name = name + "_one", srcs = ["one.proto"])
    util.helper_target(cc_proto_library, name = name + "_one_cc_proto", deps = [":" + name + "_one"])
    util.helper_target(cc_library, name = name + "_two", srcs = [":" + name + "_one_cc_proto"])

    analysis_test(
        name = name,
        target = ":" + name + "_two",
        impl = _test_proto_library_in_srcs_impl,
    )

def _test_proto_library_in_srcs_impl(env, target):
    # Just verifying that it doesn't fail to configure
    pass
