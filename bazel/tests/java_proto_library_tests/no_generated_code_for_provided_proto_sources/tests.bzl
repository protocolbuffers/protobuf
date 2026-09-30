"""Tests that java_proto_library doesn't generate code for protos provided by the runtime.

java_proto_library must not generate code for .proto files that are already
compiled into the proto runtime (listed in the toolchain's `denylisted_protos`),
otherwise there would be one-definition-rule violations. The descriptor proto is
denylisted by the default Java proto toolchain.
"""

load("@rules_testing//lib:util.bzl", "TestingAspectInfo")

def _test_no_generated_code_for_provided_proto_sources(env, target):
    outputs = [
        output.short_path
        for action in target[TestingAspectInfo].actions
        for output in action.outputs.to_list()
    ]
    env.expect.that_collection(outputs).not_contains(
        "{}/lib{}-speed.jar".format(target.label.package, target.label.name),
    )

TESTS = {
    "//:descriptor_proto": [_test_no_generated_code_for_provided_proto_sources],
}
