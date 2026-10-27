# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for same version compiler arguments."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_same_version_compiler_arguments(env, target):
    genproto = env.expect.that_target(target).action_named("GenProto")
    genproto.argv().contains("{package}/baz.proto")
    genproto.argv().contains_at_least([
        "--java_out={bindir}/{package}/baz_proto-speed-src.jar",
        "-I.",
        "{package}/baz.proto",
    ]).in_order()
    genproto.argv().not_contains("--java_out=shared,immutable:{bindir}/{package}/foo_proto-speed-src.jar")
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_runtime_jars().contains_at_least([
        "{package}/libbaz_proto-speed.jar",
        "java/core/libcore.jar",
    ])

TESTS = {
    ":baz_proto": [_test_same_version_compiler_arguments],
}
