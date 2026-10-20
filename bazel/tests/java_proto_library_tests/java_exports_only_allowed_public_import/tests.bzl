"""Tests that java_proto_library only exposes exported proto dependencies as direct dependencies."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")

def _test_java_exports_only_allowed_public_import_proto(env, target):
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.compile_jars().contains_exactly([
        "{package}/libtop_proto-speed-hjar.jar",
        "{package}/libexported1_proto-speed-hjar.jar",
        "{package}/libexported2_proto-speed-hjar.jar",
    ])

def _test_java_exports_only_allowed_public_import(env, target):
    javac = env.expect.that_target(target).action_named("Javac")
    javac.argv().contains_at_least([
        "--direct_dependencies",
        "{bindir}/{package}/libtop_proto-speed-hjar.jar",
        "{bindir}/{package}/libexported1_proto-speed-hjar.jar",
        "{bindir}/{package}/libexported2_proto-speed-hjar.jar",
    ]).in_order()

TESTS = {
    ":top_java_proto": [_test_java_exports_only_allowed_public_import_proto],
    ":java_lib": [_test_java_exports_only_allowed_public_import],
}
