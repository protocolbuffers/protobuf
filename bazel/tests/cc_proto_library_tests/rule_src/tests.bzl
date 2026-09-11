"""Tests for cc_proto_library with rule sources."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_testing//lib:truth.bzl", "matching")

def _test_rule_src_cc_proto_library(env, target):
    # Should produce C++ outputs (headers and sources).
    default_outputs = env.expect.that_target(target).default_outputs()
    default_outputs.contains_predicate(
        matching.file_basename_equals("rule_src.pb.h"),
    )
    default_outputs.contains_predicate(
        matching.file_basename_equals("rule_src.pb.cc"),
    )

    # Should provide headers in compilation_context.
    env.expect.that_depset_of_files(target[CcInfo].compilation_context.headers).contains_predicate(
        matching.file_basename_equals("rule_src.pb.h"),
    )

def _test_rule_src_cc_binary_linker_inputs(env, target):
    inputs = env.expect.that_target(target).action_named("CppLink").inputs()

    # Should depend on C++ outputs/libraries of the rule_src_proto.
    inputs.contains_predicate(
        matching.any(
            matching.file_path_matches("librule_src_proto.a"),
            matching.file_path_matches("rule_src_proto.lib"),
            matching.file_path_matches("rule_src.pb"),
        ),
    )

TESTS = {
    ":rule_src_cc_proto": [_test_rule_src_cc_proto_library],
    ":cctest_rule_src": [_test_rule_src_cc_binary_linker_inputs],
}
