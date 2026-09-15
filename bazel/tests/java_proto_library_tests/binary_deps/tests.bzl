"""Tests that java_proto_library depends on the right set of files."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_binary_deps(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)

    # Should depend on Java outputs.
    java_info.transitive_source_jars_in_package().contains_exactly([
        "{package}/foo_proto-speed-src.jar",
        "{package}/baz_proto-speed-src.jar",
    ])
    java_info.transitive_source_jars().contains_at_least([
        "java/core/liblite_runtime_only-src.jar",
        "java/core/libcore-src.jar",
    ])

    # Should depend on Java libraries.
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/libfoo_proto-speed-hjar.jar",
        "{package}/libbaz_proto-speed-hjar.jar",
    ])
    java_info.transitive_compile_time_jars().contains_at_least([
        "java/core/liblite_runtime_only-hjar.jar",
        "java/core/libcore-hjar.jar",
    ])

TESTS = {
    ":foo_java_proto": [_test_binary_deps],
}
