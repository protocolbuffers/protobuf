"""Tests for proto_library not providing CcInfo."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_testing//lib:truth.bzl", "matching")

def _test_not_providing_cc_info(env, target):
    # Verify that proto_library does not provide CcInfo.
    env.expect.that_bool(CcInfo in target).equals(False)

    # Verify that DefaultInfo does not contain C++ generated files.
    default_outputs = env.expect.that_target(target).default_outputs()
    default_outputs.not_contains_predicate(matching.file_path_matches("*.pb.h"))
    default_outputs.not_contains_predicate(matching.file_path_matches("*.pb.cc"))

TESTS = {
    ":a": [_test_not_providing_cc_info],
}
