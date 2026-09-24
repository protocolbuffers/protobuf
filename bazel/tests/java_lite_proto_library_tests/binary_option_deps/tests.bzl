"""Tests that java_binaries which depend on proto_libraries depend on the right set of files for option_deps."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_binary_option_deps(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)

    # Should depend on Java outputs, excluding option_deps.
    java_info.transitive_source_jars_in_package().contains_exactly([
        "{package}/foo_proto-lite-src.jar",
    ])
    java_info.transitive_source_jars().contains_at_least([
        "java/core/liblite-src.jar",
        "java/core/liblite_runtime_only-src.jar",
    ])

    # Should depend on Java libraries.
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/libfoo_proto-lite-hjar.jar",
    ])
    java_info.transitive_compile_time_jars().contains_at_least([
        "java/core/liblite-hjar.jar",
        "java/core/liblite_runtime_only-hjar.jar",
    ])

TESTS = {
    ":foo_java_proto_lite": [_test_binary_option_deps],
}
