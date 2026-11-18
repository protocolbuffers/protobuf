# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests that java_proto_library builds a compiled jar."""

def _test_proto_library_builds_compiled_jar(env, target):
    env.expect.that_target(target).default_outputs().contains(
        "{package}/libfoo_proto-speed.jar",
    )

TESTS = {
    ":foo_java_proto": [_test_proto_library_builds_compiled_jar],
}
