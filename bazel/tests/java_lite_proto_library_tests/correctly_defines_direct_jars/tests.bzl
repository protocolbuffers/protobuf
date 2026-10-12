"""Tests that a java_lite_proto_library only provides direct jars corresponding on the
   * proto_library rules it directly depends on, excluding anything that the proto_library rules
   * depends on themselves. This does not concern strict-deps in the compilation of the generated
   * Java code itself, only compilation of regular code in java_library/java_binary and similar
   * rules."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_foo_correctly_defines_direct_jars(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.compile_jars().contains_exactly([
        "{package}/libfoo_proto-lite-hjar.jar",
    ])
    java_info.source_jars().contains_exactly([
        "{package}/foo_proto-lite-src.jar",
    ])

def _test_bar_correctly_defines_direct_jars(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.compile_jars().contains_exactly([
        "{package}/libbar_proto-lite-hjar.jar",
    ])
    java_info.source_jars().contains_exactly([
        "{package}/bar_proto-lite-src.jar",
    ])

TESTS = {
    ":foo_java_proto_lite": [_test_foo_correctly_defines_direct_jars],
    ":bar_java_proto_lite": [_test_bar_correctly_defines_direct_jars],
}
