"""Tests for cc_proto_library with blacklisted protos in transitive deps."""

load("@rules_testing//lib:truth.bzl", "matching")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "get_cc_info_artifacts")

def _test_blacklisted_protos_in_transitive_deps(env, target):
    artifacts = get_cc_info_artifacts(target)

    # Filter out checked-in source headers and virtual include symlinks from the
    # C++ runtime (//src/google/protobuf:protobuf) to verify generated headers
    # from proto_library targets across all packages.
    generated_headers = [
        f
        for f in artifacts.compilation_context_headers
        if not f.is_source and f.owner.name != "protobuf"
    ]

    env.expect.that_collection(generated_headers).transform(
        filter = matching.file_path_matches("*.pb.h"),
    ).contains_exactly_predicates([
        matching.file_path_matches("/foo.pb.h"),
    ])

TESTS = {
    ":foo_cc_proto": [_test_blacklisted_protos_in_transitive_deps],
}
