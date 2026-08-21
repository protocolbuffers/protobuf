# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `proto_common.check_collocated` function."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")

def proto_common_check_collocated_test_suite(name):
    test_suite(
        name = name,
        tests = [
            _test_allowed_in_different_package,
            _test_disallowed_in_different_package,
            _test_export_not_allowed,
            _test_export_allowed,
        ],
    )

def _impl(ctx):
    proto_common.check_collocated(
        ctx.label,
        ctx.attr.proto_dep[ProtoInfo],
        ctx.attr.toolchain[ProtoLangToolchainInfo],
    )
    return None

check_collocated = rule(
    _impl,
    attrs = {
        "proto_dep": attr.label(),
        "toolchain": attr.label(default = "//bazel/tests/testdata:toolchain"),
    },
)

# Verifies `proto_common.check_collocated` call.
def _test_allowed_in_different_package(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "_toolchain",
        allowlist_different_package = name + "_allowed",
        command_line = "",
    )
    native.package_group(
        name = name + "_allowed",
        packages = ["//..."],
    )
    util.helper_target(
        check_collocated,
        name = name + "_check_collocated",
        proto_dep = "//bazel/tests/testdata:simple_proto",
        toolchain = name + "_toolchain",
    )

    analysis_test(
        name = name,
        target = name + "_check_collocated",
        impl = _test_allowed_in_different_package_impl,
    )

def _test_allowed_in_different_package_impl(env, target):
    env.expect.that_target(target).failures().has_size(0)

# Verifies `proto_common.check_collocated` call, when disallowed on toolchain.
def _test_disallowed_in_different_package(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "_toolchain",
        allowlist_different_package = name + "_disallowed",
        command_line = "",
    )
    native.package_group(
        name = name + "_disallowed",
        packages = [],
    )
    util.helper_target(
        check_collocated,
        name = name + "_check_collocated",
        proto_dep = "//bazel/tests/testdata:simple_proto",
        toolchain = name + "_toolchain",
    )

    analysis_test(
        name = name,
        target = name + "_check_collocated",
        impl = _test_disallowed_in_different_package_impl,
        expect_failure = True,
    )

def _test_disallowed_in_different_package_impl(env, target):
    env.expect.that_target(target).failures().contains_predicate(
        matching.str_matches(
            "Error in fail: lang_proto_library '*_check_collocated'" +
            " may only be created in the same package as proto_library " +
            "'*/testdata:simple_proto'",
        ),
    )

# Verifies `proto_common.check_collocated` call, when disallowed on proto.
def _test_export_not_allowed(name):
    util.helper_target(
        check_collocated,
        name = name + "_check_collocated",
        proto_dep = "//bazel/tests/testdata:disallow_exports_proto",
    )

    analysis_test(
        name = name,
        target = name + "_check_collocated",
        impl = _test_export_not_allowed_impl,
        expect_failure = True,
    )

def _test_export_not_allowed_impl(env, target):
    env.expect.that_target(target).failures().contains_predicate(
        matching.str_matches(
            "Error in fail: lang_proto_library '*_check_collocated'" +
            " may only be created in the same package as proto_library " +
            "'*/testdata:disallow_exports_proto'",
        ),
    )

# Verifies `proto_common.check_collocated` call, when allowed on proto.
def _test_export_allowed(name):
    util.helper_target(
        check_collocated,
        name = name + "_check_collocated",
        proto_dep = "//bazel/tests/testdata:allow_exports_proto",
    )

    analysis_test(
        name = name,
        target = name + "_check_collocated",
        impl = _test_export_allowed_impl,
    )

def _test_export_allowed_impl(env, target):
    env.expect.that_target(target).failures().has_size(0)
