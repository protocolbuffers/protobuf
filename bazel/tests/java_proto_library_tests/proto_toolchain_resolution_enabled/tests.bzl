"""Tests for protoToolchainResolution_enabled"""

def _test_out(env, target):
    env.expect.that_target(target).default_outputs().contains("{package}/libok_proto-speed.jar")

TESTS = {
    ":ok_java_proto": [_test_out],
}
