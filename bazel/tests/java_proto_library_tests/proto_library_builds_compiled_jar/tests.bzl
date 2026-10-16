"""Tests that java_proto_library builds a compiled jar."""

def _test_proto_library_builds_compiled_jar(env, target):
    env.expect.that_target(target).default_outputs().contains(
        "{package}/libfoo_proto-speed.jar",
    )

TESTS = {
    ":foo_java_proto": [_test_proto_library_builds_compiled_jar],
}
