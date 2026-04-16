"""Tests for same version compiler arguments."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_same_version_compiler_arguments(env, target):
    genproto = env.expect.that_target(target).action_named("GenProto")
    genproto.argv().contains("{package}/baz.proto")
    genproto.argv().contains_at_least([
        "--java_out=lite:{bindir}/{package}/baz_proto-lite-src.jar",
        "-I.",
        "{package}/baz.proto",
    ]).in_order()
    genproto.argv().not_contains("--java_out=lite,immutable:{bindir}/{package}/foo_proto-lite-src.jar")
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_runtime_jars().contains_at_least([
        "{package}/libbaz_proto-lite.jar",
        "java/core/liblite.jar",
        "java/core/liblite_runtime_only.jar",
    ])

TESTS = {
    ":baz_proto": [_test_same_version_compiler_arguments],
}
