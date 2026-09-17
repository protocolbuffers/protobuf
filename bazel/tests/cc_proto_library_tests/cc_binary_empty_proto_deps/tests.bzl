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
