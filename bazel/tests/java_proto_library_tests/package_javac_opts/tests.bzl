"""Tests tracking package javac opts on java_proto_library."""

def _test_package_javac_opts(env, target):
    action = env.expect.that_target(target).action_generating("{package}/libinput_proto-speed.jar")
    action.argv().contains("-packageJavacopt")

TESTS = {
    ":input_proto": [_test_package_javac_opts],
}
