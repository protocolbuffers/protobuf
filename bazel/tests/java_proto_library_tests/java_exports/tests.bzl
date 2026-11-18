# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for java_proto_library exports."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_java_exports(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_compile_time_jars_in_package().contains_at_least([
        "{package}/libtop_proto-speed-hjar.jar",
        "{package}/libexported1_proto-speed-hjar.jar",
        "{package}/libexported2_proto-speed-hjar.jar",
    ])

TESTS = {
    ":java_lib": [_test_java_exports],
}
