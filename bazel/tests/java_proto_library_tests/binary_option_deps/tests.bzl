"""Tests that java_binaries which depend on proto_libraries depend on the right set of files for option_deps."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_binary_option_deps(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_source_jars_in_package().contains_exactly([
        "{package}/foo_proto-speed-src.jar",
    ])
    java_info.transitive_source_jars().contains_at_least([
        "java/core/liblite_runtime_only-src.jar",
        "java/core/libcore-src.jar",
    ])
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/libfoo_proto-speed-hjar.jar",
    ])
    java_info.transitive_compile_time_jars().contains_at_least([
        "java/core/liblite_runtime_only-hjar.jar",
        "java/core/libcore-hjar.jar",
    ])

TESTS = {
    ":foo_java_proto": [_test_binary_option_deps],
}
