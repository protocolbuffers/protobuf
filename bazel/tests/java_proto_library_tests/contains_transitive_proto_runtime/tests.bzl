"""Tests that java_proto_library contains transitive proto runtime jars."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_contains_transitive_proto_runtime(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_runtime_jars().contains_at_least([
        "{package}/libfoo_proto-speed.jar",
        "{package}/libbar_proto-speed.jar",
        "java/core/libcore.jar",
    ])

TESTS = {
    ":foo_java_proto": [_test_contains_transitive_proto_runtime],
}
