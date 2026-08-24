"""Tests tracking javac opts on java_lite_proto_library."""

def _test_javac_opts(env, target):
    action = env.expect.that_target(target).action_generating("{package}/libinput_proto-lite.jar")
    action.argv().contains("-protoMarkerForTest")

TESTS = {
    ":input_proto": [_test_javac_opts],
}
