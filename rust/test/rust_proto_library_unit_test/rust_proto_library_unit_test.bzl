"""This module contains unit tests for rust_proto_library and its aspect."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")
load("//bazel:proto_library.bzl", "proto_library")
load("//rust:aspects.bzl", "RustProtoInfo")
load("//rust:defs.bzl", "rust_cc_proto_library", "rust_upb_proto_library")
load(":defs.bzl", "ActionsInfo", "attach_cc_aspect", "attach_upb_aspect")

def _find_actions_with_mnemonic(actions, mnemonic):
    actions = [a for a in actions if a.mnemonic == mnemonic]
    if not actions:
        fail("Couldn't find action with mnemonic {} among {}".format(
            mnemonic,
            [a.mnemonic for a in actions],
        ))
    return actions

def _check_crate_mapping(actions, target_name):
    fw_actions = _find_actions_with_mnemonic(actions, "FileWrite")
    crate_mapping_action = None
    for a in fw_actions:
        outputs = a.outputs.to_list()
        output = [o for o in outputs if o.basename == target_name + ".rust_crate_mapping"]
        if output:
            crate_mapping_action = a
    if not crate_mapping_action:
        fail("Couldn't find action outputting {}.rust_crate_mapping among {}".format(
            target_name,
            fw_actions,
        ))
    expected_content = """grand_parent_proto
2
rust/test/rust_proto_library_unit_test/grandparent1.proto
rust/test/rust_proto_library_unit_test/grandparent2.proto
parent_proto
1
rust/test/rust_proto_library_unit_test/parent.proto
parent2_proto
1
rust/test/rust_proto_library_unit_test/parent2.proto
"""
    if crate_mapping_action.content != expected_content:
        fail("The crate mapping file content didn't match. Was: {}".format(
            crate_mapping_action.content,
        ))

    protoc_action = [
        a
        for a in actions
        for i in a.inputs.to_list()
        if "rust_crate_mapping" in i.basename
    ]
    if not protoc_action:
        fail("Couldn't find action with the rust_crate_mapping as input")
    if protoc_action[0].mnemonic != "GenProto":
        fail(
            "Action that had rust_crate_mapping as input wasn't a GenProto action, but {}",
            protoc_action[0].mnemonic,
        )

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
    cc_info = rust_proto_info.dep_variant_infos[0].cc_info
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

def _rust_upb_aspect_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)
    actions = target_under_test[ActionsInfo].actions
    rustc_action = _find_actions_with_mnemonic(actions, "Rustc")[0]

    # The protoc action needs to be given the crate mapping file
    _check_crate_mapping(actions, "child_proto")

    # The action needs to have the Rust runtime as an input
    _find_rust_lib_input(rustc_action.inputs, "protobuf")

    # The action needs to have rlibs for direct deps
    _find_rust_lib_input(rustc_action.inputs, "parent")
    _find_rust_lib_input(rustc_action.inputs, "parent2")

    # The action needs to produce a .rlib artifact (sometimes .rmeta as well, not tested here).
    asserts.true(env, rustc_action.outputs.to_list()[0].path.endswith(".rlib"))

    # The aspect needs to provide CcInfo that passes UPB gencode to the eventual linking.
    _find_linker_input(target_under_test[RustProtoInfo], "child.upb_minitable")
    _find_linker_input(target_under_test[RustProtoInfo], "parent.upb_minitable")

    return analysistest.end(env)

rust_upb_aspect_test = analysistest.make(_rust_upb_aspect_test_impl)

def _test_upb_aspect():
    attach_upb_aspect(name = "child_proto_with_upb_aspect", dep = ":child_proto")

    rust_upb_aspect_test(
        name = "rust_upb_aspect_test",
        target_under_test = ":child_proto_with_upb_aspect",
    )

####################################################################################################
def _rust_cc_aspect_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)
    actions = target_under_test[ActionsInfo].actions
    rustc_action = _find_actions_with_mnemonic(actions, "Rustc")[0]

    _check_crate_mapping(actions, "child_proto")

    # The rustc action needs to have the Rust runtime as an input
    _find_rust_lib_input(rustc_action.inputs, "protobuf")

    # The action needs to have rlibs for direct deps
    _find_rust_lib_input(rustc_action.inputs, "parent")
    _find_rust_lib_input(rustc_action.inputs, "parent2")

    # The action needs to produce a .rlib artifact (sometimes .rmeta as well, not tested here).
    asserts.true(env, rustc_action.outputs.to_list()[0].path.endswith(".rlib"))

    # The aspect needs to provide CcInfo that passes UPB gencode to the eventual linking.
    _find_linker_input(target_under_test[RustProtoInfo], "child.pb")
    _find_linker_input(target_under_test[RustProtoInfo], "parent.pb")

    return analysistest.end(env)

rust_cc_aspect_test = analysistest.make(_rust_cc_aspect_test_impl)

def _test_cc_aspect():
    attach_cc_aspect(name = "child_proto_with_cc_aspect", dep = ":child_proto")

    rust_cc_aspect_test(
        name = "rust_cc_aspect_test",
        target_under_test = ":child_proto_with_cc_aspect",
    )

####################################################################################################

def _rust_outputs_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)

    label_to_files = {
        "child_cpp_rust_proto": ["generated.c.rs", "child.c.pb.rs"],
        "child_upb_rust_proto": ["generated.u.rs", "child.u.pb.rs"],
    }
    expected_outputs = label_to_files[target_under_test.label.name]
    actual_outputs = target_under_test.files.to_list()
    asserts.equals(env, len(expected_outputs), len(actual_outputs))
    for expected in expected_outputs:
        found = False
        for actual in actual_outputs:
            if actual.path.endswith(expected):
                found = True
                break
        asserts.true(env, found)

    return analysistest.end(env)

rust_outputs_test = analysistest.make(_rust_outputs_test_impl)

def _test_cc_outputs():
    rust_cc_proto_library(
        name = "child_cpp_rust_proto",
        deps = [":child_proto"],
    )

    rust_outputs_test(
        name = "rust_cc_outputs_test",
        target_under_test = ":child_cpp_rust_proto",
    )

def _test_upb_outputs():
    rust_upb_proto_library(
        name = "child_upb_rust_proto",
        deps = [":child_proto"],
    )

    rust_outputs_test(
        name = "rust_upb_outputs_test",
        target_under_test = ":child_upb_rust_proto",
    )

def rust_proto_library_unit_test(name):
    """Sets up rust_proto_library_unit_test test suite.

    Args:
      name: name of the test suite"""
    proto_library(
        # Use a '-' in the target name to test that its replaced by a '_' in the crate name.
        name = "grand-parent_proto",
        srcs = ["grandparent1.proto", "grandparent2.proto"],
    )
    proto_library(
        name = "parent_proto",
        srcs = ["parent.proto"],
        deps = [":grand-parent_proto"],
    )
    proto_library(name = "parent2_proto", srcs = ["parent2.proto"])
    proto_library(
        name = "child_proto",
        srcs = ["child.proto"],
        deps = [":parent_proto", ":parent2_proto"],
    )

    _test_upb_aspect()
    _test_cc_aspect()
    _test_cc_outputs()
    _test_upb_outputs()

    native.test_suite(
        name = name,
        tests = [
            ":rust_upb_aspect_test",
            ":rust_cc_aspect_test",
            ":rust_cc_outputs_test",
            ":rust_upb_outputs_test",
        ],
    )
