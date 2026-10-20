"""Tests that cc_proto_library can be loaded and invoked through a macro."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")

def cc_proto_library_macro(**attrs):
    cc_proto_library(**attrs)

def _test_cc_proto_library_loaded_through_macro(env, target):
    env.expect.that_target(target).has_provider(CcInfo)

TESTS = {
    ":a": [_test_cc_proto_library_loaded_through_macro],
}
