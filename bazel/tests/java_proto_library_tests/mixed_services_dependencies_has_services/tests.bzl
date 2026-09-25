"""Tests that java_proto_library targets sharing direct and indirect proto dependencies with has_services=True analyze without conflicting actions."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_a_java_proto(env, target):
    env.expect.that_target(target).default_outputs().contains("{package}/liba_proto-speed.jar")
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/liba_proto-speed-hjar.jar",
        "{package}/libcommon_proto-speed-hjar.jar",
    ])

def _test_b_java_proto(env, target):
    env.expect.that_target(target).default_outputs().contains("{package}/libb_proto-speed.jar")
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/libb_proto-speed-hjar.jar",
        "{package}/liba_proto-speed-hjar.jar",
        "{package}/libcommon_proto-speed-hjar.jar",
    ])

def _test_both_java_lib(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/libboth_java_lib-hjar.jar",
        "{package}/liba_proto-speed-hjar.jar",
        "{package}/libb_proto-speed-hjar.jar",
        "{package}/libcommon_proto-speed-hjar.jar",
    ])

TESTS = {
    ":a_java_proto": [_test_a_java_proto],
    ":b_java_proto": [_test_b_java_proto],
    ":both_java_lib": [_test_both_java_lib],
}
