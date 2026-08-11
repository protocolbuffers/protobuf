"""Tests for protoToolchainResolution_enabled"""

def _test_out(env, target):
    env.expect.that_target(target).default_outputs().contains("{package}/libok_proto-lite.jar")

TESTS = {
    ":ok_java_proto_lite": [_test_out],
}
