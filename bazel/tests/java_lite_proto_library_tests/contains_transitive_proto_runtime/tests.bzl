"""Tests that java_lite_proto_library contains transitive proto runtime jars."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_contains_transitive_proto_runtime(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_runtime_jars().contains_at_least([
        "{package}/libfoo_proto-lite.jar",
        "java/core/liblite.jar",
        "java/core/liblite_runtime_only.jar",
    ])

TESTS = {
    ":foo_java_proto_lite": [_test_contains_transitive_proto_runtime],
}
