"""Tests for java_lite_proto_library correctly defining direct jars with alias proto."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_correctly_defines_direct_jars_strict_deps_enabled_alias_proto(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.compile_jars().contains_exactly([
        "{package}/libbar_proto-lite-hjar.jar",
    ])

TESTS = {
    ":foo_proto": [_test_correctly_defines_direct_jars_strict_deps_enabled_alias_proto],
}
