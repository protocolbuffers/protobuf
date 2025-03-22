# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `proto_common.compile` function."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching", "subjects", "truth")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests/testdata:should_generate.bzl", "BoolInfo", "should_generate_rule")

def proto_common_should_generate_test_suite(name):
    util.helper_target(
        proto_library,
        name = "test_proto",
        srcs = ["A.proto"],
    )
    test_suite(
        name = name,
        tests = [
            _test_should_generate_basic,
            _test_should_generate_dont_generate,
            _test_should_generate_mixed,
        ],
    )

# Verifies `proto_common.should_generate_code` call.
def _test_should_generate_basic(name):
    util.helper_target(
        should_generate_rule,
        name = name + "_should_generate",
        proto_dep = ":test_proto",
    )

    analysis_test(
        name = name,
        target = name + "_should_generate",
        impl = _test_should_generate_basic_impl,
    )

def _test_should_generate_basic_impl(env, target):
    bool_info.from_target(env, target).value().equals(True)

# Verifies `proto_common.should_generate_code` call.
def _test_should_generate_dont_generate(name):
    util.helper_target(
        should_generate_rule,
        name = name + "_should_generate",
        proto_dep = "//bazel/tests/testdata:denied",
    )

    analysis_test(
        name = name,
        target = name + "_should_generate",
        impl = _test_should_generate_dont_generate_impl,
    )

def _test_should_generate_dont_generate_impl(env, target):
    bool_info.from_target(env, target).value().equals(False)

# Verifies `proto_common.should_generate_code` call.
def _test_should_generate_mixed(name):
    util.helper_target(
        should_generate_rule,
        name = name + "_should_generate",
        proto_dep = "//bazel/tests/testdata:mixed",
    )

    analysis_test(
        name = name,
        target = name + "_should_generate",
        impl = _test_should_generate_mixed_impl,
        expect_failure = True,
    )

def _test_should_generate_mixed_impl(env, target):
    failures = env.expect.that_target(target).failures()
    failures.contains_predicate(
        matching.str_matches(
            "The 'srcs' attribute of '*:mixed' contains protos for which 'MyRule' " +
            "shouldn't generate code (*/descriptor.proto, */metadata.proto)," +
            " in addition to protos for which it should (*/something.proto).\n" +
            "Separate '*:mixed' into 2 proto_library rules.",
        ),
    )

# Utility functions

def _new_bool_info_subject(bool_info, meta):
    self = struct(actual = bool_info, meta = meta.derive("BoolInfo"))
    public = struct(
        value = lambda: subjects.bool(getattr(bool_info, "value", False), self.meta.derive("value")),
    )
    return public

def _bool_info_from_target(env, target):
    return _new_bool_info_subject(target[BoolInfo], meta = truth.expect(env).meta.derive(
        format_str_kwargs = {
            "name": target.label.name,
            "package": target.label.package,
        },
    ))

bool_info = struct(
    from_target = _bool_info_from_target,
)
