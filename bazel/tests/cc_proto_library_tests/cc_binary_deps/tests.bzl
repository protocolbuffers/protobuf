"""Tests for cc_binary_deps."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_testing//lib:truth.bzl", "matching")

def _test_proto_library_direct_files_deps(env, target):
    # Should depend on C++ outputs (pb.cc).
    default_outputs = env.expect.that_target(target).default_outputs()
    default_outputs.contains_predicate(matching.file_basename_equals("foo.pb.cc"))
    default_outputs.contains_predicate(matching.file_basename_equals("bar.pb.cc"))

def _test_proto_library_headers_deps(env, target):
    # Should depend on C++ outputs (pb.h) directly and transitively.
    headers = env.expect.that_depset_of_files(target[CcInfo].compilation_context.headers)
    headers.contains_predicate(matching.file_basename_equals("foo.pb.h"))
    headers.contains_predicate(matching.file_basename_equals("bar.pb.h"))
    headers.contains_predicate(matching.file_basename_equals("baz.pb.h"))

def _test_cc_binary_linker_files_deps(env, target):
    inputs = env.expect.that_target(target).action_named("CppLink").inputs()

    # Should depend on C++ outputs/libraries of direct and transitive proto dependencies.
    inputs.contains_predicate(
        matching.any(
            matching.file_path_matches("libfoo_proto.a"),
            matching.file_path_matches("foo_proto.lib"),
            matching.file_path_matches("foo.pb"),
        ),
    )
    inputs.contains_predicate(
        matching.any(
            matching.file_path_matches("libbaz_proto.a"),
            matching.file_path_matches("baz_proto.lib"),
            matching.file_path_matches("baz.pb"),
        ),
    )

TESTS = {
    ":foo_cc_proto": [
        _test_proto_library_direct_files_deps,
        _test_proto_library_headers_deps,
    ],
    ":cctest_binary": [_test_cc_binary_linker_files_deps],
}
