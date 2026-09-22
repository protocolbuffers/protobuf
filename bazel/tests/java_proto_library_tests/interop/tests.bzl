"""Tests tracking Java interop behaviors."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_interop(env, target):
    env.expect.that_target(target).has_provider(JavaInfo)

TESTS = {
    ":file_java_proto": [_test_interop],
}
