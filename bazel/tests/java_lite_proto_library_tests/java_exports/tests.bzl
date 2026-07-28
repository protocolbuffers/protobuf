"""Tests for java_lite_proto_library exports."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_java_lib_depends_on_exports(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_compile_time_jars_in_package().contains_at_least([
        "{package}/libtop-lite-hjar.jar",
        "{package}/libexported1-lite-hjar.jar",
        "{package}/libexported2-lite-hjar.jar",
    ])

TESTS = {
    ":java_lib": [_test_java_lib_depends_on_exports],
}
