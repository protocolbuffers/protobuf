"""Tests that cc_proto_library can be used from cc rules."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _test_can_be_used_from_cc_library(env, target):
    env.expect.that_target(target).has_provider(CcInfo)

def _test_can_be_used_from_cc_binary(env, target):
    env.expect.that_target(target).has_provider(CcInfo)

TESTS = {
    ":foo": [_test_can_be_used_from_cc_library],
    ":bin": [_test_can_be_used_from_cc_binary],
}
