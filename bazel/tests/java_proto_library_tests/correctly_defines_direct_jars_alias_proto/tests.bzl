"""Tests that a java_proto_library only provides direct jars corresponding to the
proto_library rules it directly depends on, excluding anything that the proto_library rules
depend on themselves. This does not concern strict-deps in the compilation of the generated
Java code itself, only compilation of regular code in java_library/java_binary and similar
rules.
"""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_correctly_defines_direct_jars_alias_proto(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.compile_jars().contains_exactly([
        "{package}/libbar_proto-speed-hjar.jar",
    ])

TESTS = {
    ":foo_java_proto": [_test_correctly_defines_direct_jars_alias_proto],
}
