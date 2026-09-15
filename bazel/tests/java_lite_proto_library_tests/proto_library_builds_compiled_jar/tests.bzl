"""Tests that java_lite_proto_library builds a compiled jar."""

def _test_lite_proto_library_builds_compiled_jar(env, target):
    env.expect.that_target(target).default_outputs().contains(
        "{package}/libfoo_proto-lite.jar",
    )

TESTS = {
    ":foo_java_proto_lite": [_test_lite_proto_library_builds_compiled_jar],
}
