# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Tests for cc_toolchain prebuilt protoc configuration."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests/testdata:compile_rule.bzl", "compile_rule")

_PREFER_PREBUILT_PROTOC = str(Label("//bazel/flags:prefer_prebuilt_protoc"))

def cc_toolchain_test_suite(name):
    test_suite(
        name = name,
        tests = [
            _test_cc_toolchain_uses_protoc_minimal_when_prefer_prebuilt_flag_unset,
            _test_cc_toolchain_uses_full_protoc_by_default,
        ],
    )

def _test_cc_toolchain_uses_protoc_minimal_when_prefer_prebuilt_flag_unset(name):
    util.helper_target(
        proto_library,
        name = name + "_proto",
        srcs = ["A.proto"],
    )
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":" + name + "_proto",
        toolchain = "//:cc_toolchain",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_cc_toolchain_uses_protoc_minimal_when_prefer_prebuilt_flag_unset_impl,
        config_settings = {_PREFER_PREBUILT_PROTOC: False},
    )

def _test_cc_toolchain_uses_protoc_minimal_when_prefer_prebuilt_flag_unset_impl(env, target):
    # Find the compile action
    action = env.expect.that_target(target).action_named("GenProto")

    # When prefer_prebuilt_protoc is False, protoc_minimal_do_not_use is set,
    # so the cc_toolchain should use protoc_minimal.
    action.argv().contains_predicate(matching.str_matches("*protoc_minimal*"))

def _test_cc_toolchain_uses_full_protoc_by_default(name):
    util.helper_target(
        proto_library,
        name = name + "_proto",
        srcs = ["A.proto"],
    )
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":" + name + "_proto",
        toolchain = "//:cc_toolchain",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_cc_toolchain_uses_full_protoc_by_default_impl,
    )

def _test_cc_toolchain_uses_full_protoc_by_default_impl(env, target):
    # Find the compile action
    action = env.expect.that_target(target).action_named("GenProto")

    # By default (prefer_prebuilt_protoc is True), protoc_minimal_do_not_use is None,
    # so the cc_toolchain should use the full protoc (not protoc_minimal).
    # The protoc path should end with "/protoc" not contain "protoc_minimal"
    action.argv().contains_predicate(matching.str_matches("*/protoc"))
    action.argv().not_contains_predicate(matching.str_matches("*protoc_minimal*"))
