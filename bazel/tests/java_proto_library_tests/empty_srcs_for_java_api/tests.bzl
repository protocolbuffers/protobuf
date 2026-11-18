# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for empty srcs interop."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_empty_srcs(env, target):
    env.expect.that_target(target).has_provider(JavaInfo)

TESTS = {
    ":empty_java_proto": [_test_empty_srcs],
}
