"""Tests for output directory of the C++ proto compile action."""

load("@rules_testing//lib:truth.bzl", "matching")

def _test_output_directory_for_proto_compile_action(env, target):
    action = env.expect.that_target(target).action_generating("{package}/bar.pb.h")
    action.argv().contains_predicate(
        matching.any(
            matching.str_matches("--cpp_out=" + env.ctx.bin_dir.path),
            matching.str_matches("--cpp_out=*:" + env.ctx.bin_dir.path),
        ),
    )

TESTS = {
    ":bar_proto": [_test_output_directory_for_proto_compile_action],
}
