# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests for cc_proto_library with alias proto_library targets."""

load("@rules_testing//lib:truth.bzl", "matching")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "get_cc_info_artifacts")

def _test_alias_protos(env, target):
    artifacts = get_cc_info_artifacts(target)

    env.expect.that_collection(artifacts.compilation_context_headers_in_package).transform(
        filter = matching.file_path_matches("*.pb.h"),
    ).contains_exactly_predicates([
        matching.file_path_matches("/foo.pb.h"),
    ])

TESTS = {
    ":foo_cc_proto": [_test_alias_protos],
}
