# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for cc_binary empty proto deps."""

load("@rules_testing//lib:truth.bzl", "matching")

def _test_cc_binary_empty_linker_files_deps(env, target):
    action = env.expect.that_target(target).action_named("CppLink")

    # Should depend on C++ outputs of the transitive dependencies of the empty proto.
    action.inputs().contains_at_least_predicates([
        matching.any(
            matching.file_path_matches("libfoo_proto.a"),
            matching.file_path_matches("foo_proto.lib"),
            matching.file_path_matches("foo.pb"),
        ),
        matching.any(
            matching.file_path_matches("libbaz_proto.a"),
            matching.file_path_matches("baz_proto.lib"),
            matching.file_path_matches("baz.pb"),
        ),
    ])

TESTS = {
    ":cctest_empty_binary": [_test_cc_binary_empty_linker_files_deps],
}
