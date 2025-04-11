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

# Verifies `proto_common.check_collocated` call.
def _test_allowed_in_different_package(name):
    analysis_test(
        name = name,
        target = "//bazel/tests/testdata/different_package_allowed:check_collocated",
        impl = _test_allowed_in_different_package_impl,
    )

def _test_allowed_in_different_package_impl(env, target):
    env.expect.that_target(target).failures().has_size(0)

def _test_disallowed_in_different_package(name):
    analysis_test(
        name = name,
        target = "//bazel/tests/testdata/different_package_disallowed:check_collocated",
        impl = _test_disallowed_in_different_package_impl,
        expect_failure = True,
    )

def _test_disallowed_in_different_package_impl(env, target):
    env.expect.that_target(target).failures().contains_predicate(
        matching.str_matches(
            "Error in fail: lang_proto_library '//*/different_package_disallowed:check_collocated'" +
            " may only be created in the same package as proto_library " +
            "'//*/testdata:simple_proto'",
        ),
    )

def _test_export_not_allowed(name):
    analysis_test(
        name = name,
        target = "//bazel/tests/testdata/different_package_allowed:check_collocated_disallow_exports",
        impl = _test_export_not_allowed_impl,
        expect_failure = True,
    )

def _test_export_not_allowed_impl(env, target):
    env.expect.that_target(target).failures().contains_predicate(
        matching.str_matches(
            "Error in fail: lang_proto_library '//*:check_collocated_disallow_exports'" +
            " may only be created in the same package as proto_library " +
            "'//*/testdata:disallow_exports_proto'",
        ),
    )

def _test_export_allowed(name):
    analysis_test(
        name = name,
        target = "//bazel/tests/testdata/different_package_allowed:check_collocated_allow_exports",
        impl = _test_export_allowed_impl,
    )

def _test_export_allowed_impl(env, target):
    env.expect.that_target(target).failures().has_size(0)
