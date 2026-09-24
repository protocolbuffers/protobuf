"""Tests that java_proto_library indirectly depending on a proto_library with has_services=True analyzes cleanly."""

load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("//bazel/common:proto_info.bzl", "ProtoInfo")

def _test_inner_proto(env, target):
    env.expect.that_target(target).has_provider(ProtoInfo)

def _test_outer_proto(env, target):
    env.expect.that_target(target).has_provider(ProtoInfo)

def _test_java_stubby_library_migration_indirect(env, target):
    env.expect.that_target(target).default_outputs().contains("{package}/libouter_proto-speed.jar")
    java_info = env.expect.that_target(target).provider(JavaInfo)
    java_info.transitive_compile_time_jars_in_package().contains_exactly([
        "{package}/libouter_proto-speed-hjar.jar",
        "{package}/libinner_proto-speed-hjar.jar",
    ])

TESTS = {
    ":inner_proto": [_test_inner_proto],
    ":outer_proto": [_test_outer_proto],
    ":outer_java_proto": [_test_java_stubby_library_migration_indirect],
}
