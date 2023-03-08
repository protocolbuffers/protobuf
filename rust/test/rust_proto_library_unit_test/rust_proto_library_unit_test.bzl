"""This module contains unit tests for rust_proto_library and its aspect."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load(":defs.bzl", "ActionsInfo", "attach_aspect")
load("//rust:defs.bzl", "RustProtoInfo")

def _find_action_with_mnemonic(actions, mnemonic):
    action = [a for a in actions if a.mnemonic == mnemonic]
    if not action:
        fail("Couldn't find action with mnemonic {} among {}".format(mnemonic, actions))
    return action[0]

def _find_rust_lib_input(inputs, target_name):
    inputs = inputs.to_list()
    input = [i for i in inputs if i.basename.startswith("lib" + target_name) and
                                  (i.basename.endswith(".rlib") or i.basename.endswith(".rmeta"))]
    if not input:
        fail("Couldn't find lib{}-<hash>.rlib or lib{}-<hash>.rmeta among {}".format(
            target_name,
            target_name,
            [i.basename for i in inputs],
        ))
    return input[0]

def _relevant_linker_inputs(ltl):
    return ltl.objects + ltl.pic_objects + (
        [ltl.static_library] if ltl.static_library else []
    ) + (
        [ltl.pic_static_library] if ltl.pic_static_library else []
    )

def _find_linker_input(rust_proto_info, basename_substring):
    cc_info = rust_proto_info.dep_variant_info.cc_info
    for linker_input in cc_info.linking_context.linker_inputs.to_list():
        for ltl in linker_input.libraries:
            for file in _relevant_linker_inputs(ltl):
                if basename_substring in file.basename:
                    return file

    fail("Couldn't find file with suffix {} in {}".format(
        basename_substring,
        [
            f.basename
            for input in cc_info.linking_context.linker_inputs.to_list()
            for ltl in input.libraries
            for f in _relevant_linker_inputs(ltl)
        ],
    ))

####################################################################################################

def _rust_aspect_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)
    actions = target_under_test[ActionsInfo].actions
    rustc_action = _find_action_with_mnemonic(actions, "Rustc")

    # The action needs to have the Rust runtime as an input
    _find_rust_lib_input(rustc_action.inputs, "protobuf")

    # The action needs to produce a .rlib artifact (sometimes .rmeta as well, not tested here).
    asserts.true(env, rustc_action.outputs.to_list()[0].path.endswith(".rlib"))

    # The aspect needs to provide CcInfo that passes UPB gencode to the eventual linking.
    _find_linker_input(target_under_test[RustProtoInfo], "child.upb.thunks")
    _find_linker_input(target_under_test[RustProtoInfo], "parent.upb.thunks")

    return analysistest.end(env)

rust_aspect_test = analysistest.make(_rust_aspect_test_impl)

def _test_aspect():
    attach_aspect(name = "child_proto_with_aspect", dep = ":child_proto")

    rust_aspect_test(
        name = "rust_aspect_test",
        target_under_test = ":child_proto_with_aspect",
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        tags = ["not_build:arm"],
    )

####################################################################################################

def rust_proto_library_unit_test(name):
    """Sets up rust_proto_library_unit_test test suite.

    Args:
      name: name of the test suite"""
    native.proto_library(name = "parent_proto", srcs = ["parent.proto"])
    native.proto_library(name = "child_proto", srcs = ["child.proto"], deps = [":parent_proto"])

    _test_aspect()

    native.test_suite(
        name = name,
        tests = [
            ":rust_aspect_test",
        ],
    )
