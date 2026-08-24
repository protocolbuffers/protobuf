"""Tests tracking javac opts on java_proto_library."""

def _test_javac_opts(env, target):
    action = env.expect.that_target(target).action_generating("{package}/libinput_proto-speed.jar")
    action.argv().contains("-protoMarkerForTest")

TESTS = {
    ":input_proto": [_test_javac_opts],
}
