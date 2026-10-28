# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests that java_proto_library doesn't pass public import flags to the protocol compiler."""

def _test_java_proto_exports_without_allowed_public_check(env, target):
    genproto = env.expect.that_target(target).action_named("GenProto")
    genproto.argv().not_contains("--allowed_public_imports")
    genproto.argv().not_contains("--allowed_public_imports=")

TESTS = {
    ":top_proto": [_test_java_proto_exports_without_allowed_public_check],
}
