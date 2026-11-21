# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests that java_proto_library doesn't generate code for protos provided by the runtime.

java_proto_library must not generate code for .proto files that are already
compiled into the proto runtime (listed in the toolchain's `denylisted_protos`),
otherwise there would be one-definition-rule violations.

The Java proto toolchain cannot be swapped for a custom one from an analysis
test (the proto_toolchain_for_java flag always forwards the default
toolchain's ProtoLangToolchainInfo), so the test applies the java_proto_library
aspect to the descriptor proto, which the default toolchain denylists, and
checks that no compiled jar is produced. As a positive control, a proto that is
not denylisted gets its compiled jar from the very same aspect.
"""

load("@rules_testing//lib:util.bzl", "TestingAspectInfo")

def _compiled_jar_outputs(target):
    return [
        output.basename
        for action in target[TestingAspectInfo].actions
        for output in action.outputs.to_list()
        if output.basename == "lib{}-speed.jar".format(target.label.name)
    ]

def _test_no_generated_code_for_provided_proto_sources(env, target):
    env.expect.that_collection(_compiled_jar_outputs(target)).contains_exactly([])

def _test_generated_code_for_not_provided_proto_sources(env, target):
    env.expect.that_collection(_compiled_jar_outputs(target)).contains_exactly(["libnot_provided_proto-speed.jar"])

TESTS = {
    "//:descriptor_proto": [_test_no_generated_code_for_provided_proto_sources],
    ":not_provided_proto": [_test_generated_code_for_not_provided_proto_sources],
}
