# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `proto_common.compile` function."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests/testdata:compile_rule.bzl", "compile_rule")

protocol_compiler = "/protoc"

def proto_common_compile_test_suite(name):
    util.helper_target(
        proto_library,
        name = "simple_proto",
        srcs = ["A.proto"],
    )
    test_suite(
        name = name,
        tests = [
            _test_compile_basic,
            _test_compile_noplugin,
            _test_compile_with_plugin_output,
            _test_compile_with_directory_plugin_output,
            _test_compile_additional_args,
            _test_compile_additional_tools,
            _test_compile_additional_tools_no_plugin,
            _test_compile_additional_inputs,
            _test_compile_resource_set,
            _test_compile_protoc_opts,
            _test_compile_direct_generated_protos,
            _test_compile_indirect_generated_protos,
        ],
    )

# Verifies basic usage of `proto_common.compile`.
def _test_compile_basic(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_basic_impl,
    )

def _test_compile_basic_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith(protocol_compiler),
            matching.str_matches("--plugin=b*-out/*-exec*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )
    action.mnemonic().equals("MyMnemonic")

# Verifies usage of proto_common.generate_code with no plugin specified by toolchain.
def _test_compile_noplugin(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
        toolchain = "//bazel/tests/testdata:toolchain_noplugin",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_noplugin_impl,
    )

def _test_compile_noplugin_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith(protocol_compiler),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `plugin_output` parameter set to file.
def _test_compile_with_plugin_output(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
        plugin_output = "single",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_with_plugin_output_impl,
    )

def _test_compile_with_plugin_output_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith(protocol_compiler),
            matching.str_matches("--java_out=param1,param2:b*-out/*/test_compile_with_plugin_output_compile"),
            matching.str_matches("--plugin=b*-out/*-exec*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `plugin_output` parameter set to file.
def _test_compile_with_directory_plugin_output(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
        plugin_output = "multiple",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_with_directory_plugin_output_impl,
    )

def _test_compile_with_directory_plugin_output_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith(protocol_compiler),
            matching.str_matches("--java_out=param1,param2:b*-out/*/bin"),
            matching.str_matches("--plugin=b*-out/*-exec*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_args` parameter
def _test_compile_additional_args(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
        additional_args = ["--a", "--b"],
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_additional_args_impl,
    )

def _test_compile_additional_args_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith(protocol_compiler),
            matching.equals_wrapper("--a"),
            matching.equals_wrapper("--b"),
            matching.str_matches("--plugin=b*-out/*-exec*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_tools` parameter
def _test_compile_additional_tools(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
        additional_tools = [
            "//bazel/tests/testdata:_tool1",
            "//bazel/tests/testdata:_tool2",
        ],
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_additional_tools_impl,
    )

def _test_compile_additional_tools_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("_tool1"),
            matching.file_basename_equals("_tool2"),
            matching.file_basename_equals("plugin"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_tools` parameter and no plugin on the toolchain.
def _test_compile_additional_tools_no_plugin(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
        additional_tools = [
            "//bazel/tests/testdata:_tool1",
            "//bazel/tests/testdata:_tool2",
        ],
        toolchain = "//bazel/tests/testdata:toolchain_noplugin",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_additional_tools_no_plugin_impl,
    )

def _test_compile_additional_tools_no_plugin_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("_tool1"),
            matching.file_basename_equals("_tool2"),
        ],
    )
    action.inputs().not_contains_predicate(matching.file_basename_equals("plugin"))

# Verifies usage of `proto_common.compile` with `additional_inputs` parameter.
def _test_compile_additional_inputs(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
        additional_inputs = ["input1.txt", "input2.txt"],
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_additional_inputs_impl,
    )

def _test_compile_additional_inputs_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("input1.txt"),
            matching.file_basename_equals("input2.txt"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_tools` parameter and no plugin on the toolchain.
def _test_compile_resource_set(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
        use_resource_set = True,
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_resource_set_impl,
    )

def _test_compile_resource_set_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")  # @unused
    # We can't check the specification of the resource set, but we at least verify analysis passes

# Verifies `--protocopts` are passed to command line.
def _test_compile_protoc_opts(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        config_settings = {"//command_line_option:protocopt": ["--foo", "--bar"]},
        impl = _test_compile_protoc_opts_impl,
    )

def _test_compile_protoc_opts_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith(protocol_compiler),
            matching.equals_wrapper("--foo"),
            matching.equals_wrapper("--bar"),
            matching.str_matches("--plugin=b*-out/*-exec*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

#  Verifies `proto_common.compile`> correctly handles direct generated `.proto` files.
def _test_compile_direct_generated_protos(name):
    util.helper_target(native.genrule, name = name + "_generate_G", cmd = "", outs = ["G.proto"])
    util.helper_target(
        proto_library,
        name = name + "_directly_generated_proto",
        srcs = ["A.proto", "G.proto"],
    )
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = name + "_directly_generated_proto",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_direct_generated_protos_impl,
    )

def _test_compile_direct_generated_protos_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith(protocol_compiler),
            matching.str_matches("--plugin=b*-out/*-exec*/bin/*/testdata/plugin"),
            matching.str_matches("-Ib*-out/*/*"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
            matching.str_matches("*-out/*/*/*/G.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `plugin_output` parameter
def _test_compile_indirect_generated_protos(name):
    util.helper_target(native.genrule, name = "_generate_h", srcs = ["A.txt"], cmd = "", outs = ["H.proto"])
    util.helper_target(proto_library, name = "_generated_proto", srcs = ["H.proto"])
    util.helper_target(
        proto_library,
        name = name + "_indirectly_generated_proto",
        srcs = ["A.proto"],
        deps = [":_generated_proto"],
    )
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = name + "_indirectly_generated_proto",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_indirect_generated_protos_impl,
    )

def _test_compile_indirect_generated_protos_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith(protocol_compiler),
            matching.str_matches("--plugin=b*-out/*-exec*/bin/*/testdata/plugin"),
            matching.str_matches("-Ib*-out/*/*"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )
