# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for proto_library not providing CcInfo."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_testing//lib:truth.bzl", "matching")

def _test_not_providing_cc_info(env, target):
    # Verify that proto_library does not provide CcInfo.
    env.expect.that_bool(CcInfo in target).equals(False)

    # Verify that DefaultInfo does not contain C++ generated files.
    default_outputs = env.expect.that_target(target).default_outputs()
    default_outputs.not_contains_predicate(matching.file_path_matches("*.pb.h"))
    default_outputs.not_contains_predicate(matching.file_path_matches("*.pb.cc"))
    default_outputs.not_contains_predicate(matching.file_path_matches("*.proto.h"))

TESTS = {
    ":a": [_test_not_providing_cc_info],
}
