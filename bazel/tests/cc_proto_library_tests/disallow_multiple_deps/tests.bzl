# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests that cc_proto_library disallows zero or multiple deps."""

load("@rules_testing//lib:truth.bzl", "matching")

def _test_multiple_deps(env, target):
    # Match both the OSS cc_proto_library error message (checked by the legacy
    # CcProtoLibraryTest in Bazel mode) and the Google3 cc_proto_library error
    # message so this portable test passes in both environments.
    env.expect.that_target(target).failures().contains_predicate(
        matching.any(
            matching.str_matches("*'deps' attribute must contain exactly one label*"),
            matching.str_matches("*more than one deps attribute found; expected only one.*"),
        ),
    )

def _test_empty_deps(env, target):
    # Match both the OSS cc_proto_library error message and the Google3
    # cc_proto_library error message for empty deps.
    env.expect.that_target(target).failures().contains_predicate(
        matching.any(
            matching.str_matches("*'deps' attribute must contain exactly one label*"),
            matching.str_matches("*no deps attribute found; expected one.*"),
        ),
    )

TESTS = {
    ":multiple_deps_cc_proto": [_test_multiple_deps],
    ":empty_deps_cc_proto": [_test_empty_deps],
}
