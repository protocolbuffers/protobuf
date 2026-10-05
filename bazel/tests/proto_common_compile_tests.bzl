# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `proto_common.compile` function."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests/testdata:compile_rule.bzl", "compile_rule")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")

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
            _test_compile_protoc_opts_starlark,
            _test_compile_direct_generated_protos,
            _test_compile_indirect_generated_protos,
            _test_compile_override_progress_message,
            _test_proto_compiler_flag_override,
            _test_proto_compiler_native_flag_override,
            _test_proto_toolchain_for_cc_flag_override,
            _test_proto_toolchain_for_cc_native_flag_override,
            _test_proto_toolchain_for_java_flag_override,
            _test_proto_toolchain_for_java_native_flag_override,
            _test_proto_toolchain_for_javalite_flag_override,
            _test_proto_toolchain_for_javalite_native_flag_override,
        ],
    )

def _match_proto_compiler():
    return matching.any(
        matching.str_endswith(protocol_compiler),
        matching.str_endswith("/protoc"),
        matching.str_endswith("/protoc.exe"),
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
            _match_proto_compiler(),
            matching.str_matches("--plugin=*out*testdata/plugin"),
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
            _match_proto_compiler(),
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
            _match_proto_compiler(),
            matching.str_matches("--java_out=param1,param2:*out*test_compile_with_plugin_output_compile"),
            matching.str_matches("--plugin=*out*testdata/plugin"),
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
            _match_proto_compiler(),
            matching.str_matches("--java_out=param1,param2:*out*bin"),
            matching.str_matches("--plugin=*out*testdata/plugin"),
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
            _match_proto_compiler(),
            matching.str_matches("--plugin=*out*testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
            matching.equals_wrapper("--a"),
            matching.equals_wrapper("--b"),
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
            matching.any(matching.file_basename_equals("_tool1"), matching.file_basename_equals("_tool1.exe")),
            matching.any(matching.file_basename_equals("_tool2"), matching.file_basename_equals("_tool2.exe")),
            matching.any(matching.file_basename_equals("plugin"), matching.file_basename_equals("plugin.exe")),
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
            matching.any(matching.file_basename_equals("_tool1"), matching.file_basename_equals("_tool1.exe")),
            matching.any(matching.file_basename_equals("_tool2"), matching.file_basename_equals("_tool2.exe")),
        ],
    )
    action.inputs().not_contains_predicate(matching.any(matching.file_basename_equals("plugin"), matching.file_basename_equals("plugin.exe")))

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
        config_settings = {
            "//command_line_option:protocopt": ["--foo", "--bar"],
        },
        impl = _test_compile_protoc_opts_impl,
    )

def _test_compile_protoc_opts_starlark(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        proto_dep = ":simple_proto",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        config_settings = {
            "@@//bazel/flags:protocopt": ["--foo", "--bar"],
        },
        impl = _test_compile_protoc_opts_impl,
    )

def _test_compile_protoc_opts_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            _match_proto_compiler(),
            matching.str_matches("--plugin=*out*testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.equals_wrapper("--foo"),
            matching.equals_wrapper("--bar"),
            matching.str_endswith("/A.proto"),
        ],
    )

#  Verifies `proto_common.compile` correctly handles direct generated `.proto` files.
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
            _match_proto_compiler(),
            matching.str_matches("--plugin=*out*testdata/plugin"),
            matching.str_matches("-I*out/*"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
            matching.str_matches("*out*G.proto"),
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
            _match_proto_compiler(),
            matching.str_matches("--plugin=*out*testdata/plugin"),
            matching.str_matches("-I*out/*"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `experimental_progress_message` parameter
def _test_compile_override_progress_message(name):
    util.helper_target(
        compile_rule,
        name = name + "_compile",
        progress_message = "My custom progress message %{label}",
        proto_dep = ":simple_proto",
    )

    analysis_test(
        name = name,
        target = name + "_compile",
        impl = _test_compile_override_progress_message_impl,
    )

def _test_compile_override_progress_message_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    env.expect.that_str(repr(action.actual)).contains("My custom progress message //")

def _dummy_compiler_impl(ctx):
    exe = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.write(exe, "#!/bin/sh\nexit 0\n", is_executable = True)
    return [DefaultInfo(executable = exe, files = depset([exe]))]

_dummy_compiler = rule(
    implementation = _dummy_compiler_impl,
    executable = True,
)

def _test_proto_compiler_flag_override(name):
    util.helper_target(
        _dummy_compiler,
        name = name + "_custom_compiler",
    )

    analysis_test(
        name = name,
        target = "@//bazel/flags:proto_compiler",
        impl = _test_proto_compiler_flag_override_impl,
        config_settings = {
            "@@//bazel/flags:proto_compiler": "//bazel/tests:" + name + "_custom_compiler",
        },
    )

def _test_proto_compiler_flag_override_impl(env, target):
    env.expect.that_target(target).has_provider(BuildSettingInfo)
    val = target[BuildSettingInfo].value
    env.expect.that_str(str(val)).contains("_custom_compiler")

def _test_proto_compiler_native_flag_override(name):
    util.helper_target(
        _dummy_compiler,
        name = name + "_custom_compiler_native",
    )

    analysis_test(
        name = name,
        target = "@//bazel/flags:proto_compiler",
        impl = _test_proto_compiler_native_flag_override_impl,
        config_settings = {
            "//command_line_option:proto_compiler": str(Label("//bazel/tests:" + name + "_custom_compiler_native")),
        },
    )

def _test_proto_compiler_native_flag_override_impl(env, target):
    env.expect.that_target(target).has_provider(BuildSettingInfo)
    val = target[BuildSettingInfo].value
    env.expect.that_str(str(val)).contains("_custom_compiler_native")

def _test_proto_toolchain_for_cc_flag_override(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "_custom_cc_toolchain",
        command_line = "$(OUT)",
        mnemonic = "CustomCcMnemonic",
    )

    analysis_test(
        name = name,
        target = "@//bazel/flags/cc:proto_toolchain_for_cc",
        impl = _test_proto_toolchain_for_cc_flag_override_impl,
        config_settings = {
            "@@//bazel/flags/cc:proto_toolchain_for_cc": "//bazel/tests:" + name + "_custom_cc_toolchain",
        },
    )

def _test_proto_toolchain_for_cc_flag_override_impl(env, target):
    env.expect.that_target(target).has_provider(BuildSettingInfo)
    val = target[BuildSettingInfo].value
    env.expect.that_str(str(val)).contains("_custom_cc_toolchain")

def _test_proto_toolchain_for_cc_native_flag_override(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "_custom_cc_toolchain_native",
        command_line = "$(OUT)",
        mnemonic = "CustomCcMnemonicNative",
    )

    analysis_test(
        name = name,
        target = "@//bazel/flags/cc:proto_toolchain_for_cc",
        impl = _test_proto_toolchain_for_cc_native_flag_override_impl,
        config_settings = {
            "//command_line_option:proto_toolchain_for_cc": str(Label("//bazel/tests:" + name + "_custom_cc_toolchain_native")),
        },
    )

def _test_proto_toolchain_for_cc_native_flag_override_impl(env, target):
    env.expect.that_target(target).has_provider(BuildSettingInfo)
    val = target[BuildSettingInfo].value
    env.expect.that_str(str(val)).contains("_custom_cc_toolchain_native")

def _test_proto_toolchain_for_java_flag_override(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "_custom_java_toolchain",
        command_line = "$(OUT)",
        mnemonic = "CustomJavaMnemonic",
    )

    analysis_test(
        name = name,
        target = "@//bazel/flags/java:proto_toolchain_for_java",
        impl = _test_proto_toolchain_for_java_flag_override_impl,
        config_settings = {
            "@@//bazel/flags/java:proto_toolchain_for_java": "//bazel/tests:" + name + "_custom_java_toolchain",
        },
    )

def _test_proto_toolchain_for_java_flag_override_impl(env, target):
    env.expect.that_target(target).has_provider(BuildSettingInfo)
    val = target[BuildSettingInfo].value
    env.expect.that_str(str(val)).contains("_custom_java_toolchain")

def _test_proto_toolchain_for_java_native_flag_override(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "_custom_java_toolchain_native",
        command_line = "$(OUT)",
        mnemonic = "CustomJavaMnemonicNative",
    )

    analysis_test(
        name = name,
        target = "@//bazel/flags/java:proto_toolchain_for_java",
        impl = _test_proto_toolchain_for_java_native_flag_override_impl,
        config_settings = {
            "//command_line_option:proto_toolchain_for_java": str(Label("//bazel/tests:" + name + "_custom_java_toolchain_native")),
        },
    )

def _test_proto_toolchain_for_java_native_flag_override_impl(env, target):
    env.expect.that_target(target).has_provider(BuildSettingInfo)
    val = target[BuildSettingInfo].value
    env.expect.that_str(str(val)).contains("_custom_java_toolchain_native")

def _test_proto_toolchain_for_javalite_flag_override(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "_custom_javalite_toolchain",
        command_line = "$(OUT)",
        mnemonic = "CustomJavaLiteMnemonic",
    )

    analysis_test(
        name = name,
        target = "@//bazel/flags/java:proto_toolchain_for_javalite",
        impl = _test_proto_toolchain_for_javalite_flag_override_impl,
        config_settings = {
            "@@//bazel/flags/java:proto_toolchain_for_javalite": "//bazel/tests:" + name + "_custom_javalite_toolchain",
        },
    )

def _test_proto_toolchain_for_javalite_flag_override_impl(env, target):
    env.expect.that_target(target).has_provider(BuildSettingInfo)
    val = target[BuildSettingInfo].value
    env.expect.that_str(str(val)).contains("_custom_javalite_toolchain")

def _test_proto_toolchain_for_javalite_native_flag_override(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "_custom_javalite_toolchain_native",
        command_line = "$(OUT)",
        mnemonic = "CustomJavaLiteMnemonicNative",
    )

    analysis_test(
        name = name,
        target = "@//bazel/flags/java:proto_toolchain_for_javalite",
        impl = _test_proto_toolchain_for_javalite_native_flag_override_impl,
        config_settings = {
            "//command_line_option:proto_toolchain_for_javalite": str(Label("//bazel/tests:" + name + "_custom_javalite_toolchain_native")),
        },
    )

def _test_proto_toolchain_for_javalite_native_flag_override_impl(env, target):
    env.expect.that_target(target).has_provider(BuildSettingInfo)
    val = target[BuildSettingInfo].value
    env.expect.that_str(str(val)).contains("_custom_javalite_toolchain_native")
