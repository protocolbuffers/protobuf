"""Tests for cc_proto_library CcCompilationContext headers."""

load("@rules_testing//lib:truth.bzl", "matching")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "get_cc_info_artifacts")

def _test_cc_compilation_context(env, target):
    artifacts = get_cc_info_artifacts(target)

    env.expect.that_collection(artifacts.compilation_context_headers_in_package).transform(
        filter = matching.file_path_matches("*.pb.h"),
    ).contains_exactly_predicates([
        matching.file_path_matches("/foo.pb.h"),
        matching.file_path_matches("/bar.pb.h"),
    ])

TESTS = {
    ":foo_cc_proto": [_test_cc_compilation_context],
}
