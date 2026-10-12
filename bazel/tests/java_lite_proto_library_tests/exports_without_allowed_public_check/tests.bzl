"""Tests that java_lite_proto_library doesn't pass public import flags to the protocol compiler."""

def _test_java_lite_proto_exports_without_allowed_public_check(env, target):
    genproto = env.expect.that_target(target).action_named("GenProto")
    genproto.argv().not_contains("--allowed_public_imports")
    genproto.argv().not_contains("--allowed_public_imports=")

TESTS = {
    ":top_proto": [_test_java_lite_proto_exports_without_allowed_public_check],
}
