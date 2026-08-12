"""Tests tracking Java interop behaviors."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_interop(env, target):
    env.expect.that_target(target).has_provider(JavaInfo)
    java_info = target[JavaInfo]

    # Verify these properties can be accessed.
    _ = java_info.transitive_compile_time_jars  # @unused
    _ = java_info.outputs  # @unused

TESTS = {
    ":file_java_proto_lite": [_test_interop],
}
