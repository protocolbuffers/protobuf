"""Tests that we pass the correct arguments to the protocol compiler."""

def _test_lite_java_proto2_compiler_args(env, target):
    genproto = env.expect.that_target(target).action_named("GenProto")
    genproto.argv().contains("{package}/foo.proto")
    genproto.argv().contains_at_least([
        "--java_out=lite:{bindir}/{package}/foo_proto-lite-src.jar",
        "-I.",
        "{package}/foo.proto",
    ]).in_order()

TESTS = {
    ":foo_proto": [_test_lite_java_proto2_compiler_args],
}
