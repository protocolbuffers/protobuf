"""Tests for empty srcs interop."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_empty_srcs(env, target):
    env.expect.that_target(target).has_provider(JavaInfo)

TESTS = {
    ":empty_java_proto_lite": [_test_empty_srcs],
}
