# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests that cc_proto_library can be loaded and invoked through a macro."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")

def cc_proto_library_macro(**attrs):
    cc_proto_library(**attrs)

def _test_cc_proto_library_loaded_through_macro(env, target):
    env.expect.that_target(target).has_provider(CcInfo)

TESTS = {
    ":a": [_test_cc_proto_library_loaded_through_macro],
}
