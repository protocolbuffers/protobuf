# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests tracking Java interop behaviors."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_java_protos_expose_starlark_providers(env, target):
    env.expect.that_target(target).has_provider(JavaInfo)
    java_info = target[JavaInfo]

    # Verify these properties can be accessed.
    _ = java_info.transitive_compile_time_jars  # @unused
    _ = java_info.outputs  # @unused

TESTS = {
    ":file_java_proto": [_test_java_protos_expose_starlark_providers],
}
