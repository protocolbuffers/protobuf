"""Tests that java_lite_proto_library passes the target label to the protocol compiler."""

def _test_lite_command_line_contains_target_label(env, target):
    javac = env.expect.that_target(target).action_named("Javac")
    javac.argv().contains_at_least([
        "--target_label",
        "//{package}:foo_proto",
        "--injecting_rule_kind",
        "java_lite_proto_library",
    ]).in_order()

TESTS = {
    ":foo_proto": [_test_lite_command_line_contains_target_label],
}
