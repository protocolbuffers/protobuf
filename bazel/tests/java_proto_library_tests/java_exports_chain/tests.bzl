"""Tests that java_proto_library propagates transitive exports chain as direct dependencies."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_java_exports_chain(env, target):
    action = env.expect.that_target(target).action_generating("{package}/libmid_proto-speed.jar")
    action.argv().contains_at_least([
        "--direct_dependencies",
        "{bindir}/{package}/libe1_proto-speed-hjar.jar",
        "{bindir}/{package}/libe2_proto-speed-hjar.jar",
        "{bindir}/{package}/libbottom_proto-speed-hjar.jar",
    ]).in_order()

    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.compile_jars().contains_exactly([
        "{package}/libmid_proto-speed-hjar.jar",
        "{package}/libe1_proto-speed-hjar.jar",
        "{package}/libe2_proto-speed-hjar.jar",
        "{package}/libbottom_proto-speed-hjar.jar",
    ])

TESTS = {
    ":mid_proto": [_test_java_exports_chain],
}
