"""Tests for `proto_common.compile` function."""

load("//bazel:proto_library.bzl", "proto_library")
load("//src/google/protobuf/github/tests/testdata:compile_rule.bzl", "compile_rule")
load("//third_party/bazel_rules/rules_testing/lib:analysis_test.bzl", "analysis_test", "test_suite")
load("//third_party/bazel_rules/rules_testing/lib:truth.bzl", "matching")
load("//third_party/bazel_rules/rules_testing/lib:util.bzl", "PREVENT_IMPLICIT_BUILDING")

def proto_common_compile_test_suite(name):
    proto_library(
        name = "simple_proto",
        srcs = ["A.proto"],
        **PREVENT_IMPLICIT_BUILDING
    )
    test_suite(
        name = name,
        tests = [
            proto_common_compile_basic,
            proto_common_compile_noplugin,
            proto_common_compile_with_plugin_output,
            proto_common_compile_with_directory_plugin_output,
            proto_common_compile_additional_args,
            proto_common_compile_additional_tools,
            proto_common_compile_additional_tools_no_pluigin,
            proto_common_compile_additional_inputs,
            proto_common_compile_resource_set,
            proto_common_compile_protoc_opts,
            proto_common_compile_direct_generated_protos,
            proto_common_compile_indirect_generated_protos,
        ],
    )

# Verifies basic usage of `proto_common.compile`.
def proto_common_compile_basic(name):
    compile_rule(
        name = "compile_basic",
        proto_dep = ":simple_proto",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_basic,
        target = ":compile_basic",
    )

def _proto_common_compile_basic(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.str_matches("--plugin=b*-out/*-exec-*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )
    action.mnemonic().equals("MyMnemonic")

# Verifies usage of proto_common.generate_code with no plugin specified by toolchain.
def proto_common_compile_noplugin(name):
    compile_rule(
        name = "compile_noplugin",
        proto_dep = ":simple_proto",
        toolchain = "//src/google/protobuf/github/tests/testdata:toolchain_noplugin",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_noplugin,
        target = ":compile_noplugin",
    )

def _proto_common_compile_noplugin(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `plugin_output` parameter set to file.
def proto_common_compile_with_plugin_output(name):
    compile_rule(
        name = "compile_with_plugin_output",
        proto_dep = ":simple_proto",
        plugin_output = "single",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_with_plugin_output,
        target = ":compile_with_plugin_output",
    )

def _proto_common_compile_with_plugin_output(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.str_matches("--java_out=param1,param2:b*-out/*/compile_with_plugin_output"),
            matching.str_matches("--plugin=b*-out/*-exec-*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `plugin_output` parameter set to file.
def proto_common_compile_with_directory_plugin_output(name):
    compile_rule(
        name = "compile_with_directory_plugin_output",
        proto_dep = ":simple_proto",
        plugin_output = "multiple",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_with_directory_plugin_output,
        target = ":compile_with_directory_plugin_output",
    )

def _proto_common_compile_with_directory_plugin_output(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.str_matches("--java_out=param1,param2:b*-out/*/bin"),
            matching.str_matches("--plugin=b*-out/*-exec-*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_args` parameter
def proto_common_compile_additional_args(name):
    compile_rule(
        name = "compile_additional_args",
        proto_dep = ":simple_proto",
        additional_args = ["--a", "--b"],
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_additional_args,
        target = ":compile_additional_args",
    )

def _proto_common_compile_additional_args(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.equals_wrapper("--a"),
            matching.equals_wrapper("--b"),
            matching.str_matches("--plugin=b*-out/*-exec-*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_tools` parameter
def proto_common_compile_additional_tools(name):
    native.cc_binary(name = "_tool1", srcs = ["tool1.cc"], **PREVENT_IMPLICIT_BUILDING)
    native.cc_binary(name = "_tool2", srcs = ["tool2.cc"], **PREVENT_IMPLICIT_BUILDING)
    compile_rule(
        name = "compile_additional_tools",
        proto_dep = ":simple_proto",
        additional_tools = [":_tool1", ":_tool2"],
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_additional_tools,
        target = ":compile_additional_tools",
    )

def _proto_common_compile_additional_tools(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("_tool1"),
            matching.file_basename_equals("_tool2"),
            matching.file_basename_equals("plugin"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_tools` parameter and no plugin on the toolchain.
def proto_common_compile_additional_tools_no_pluigin(name):
    native.cc_binary(name = "_noplugin_tool1", srcs = ["tool1.cc"], **PREVENT_IMPLICIT_BUILDING)
    native.cc_binary(name = "_noplugin_tool2", srcs = ["tool2.cc"], **PREVENT_IMPLICIT_BUILDING)
    compile_rule(
        name = "compile_additional_tools_noplugin",
        proto_dep = ":simple_proto",
        additional_tools = [":_noplugin_tool1", ":_noplugin_tool2"],
        toolchain = "//src/google/protobuf/github/tests/testdata:toolchain_noplugin",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_additional_tools_no_pluigin,
        target = ":compile_additional_tools_noplugin",
    )

def _proto_common_compile_additional_tools_no_pluigin(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("_noplugin_tool1"),
            matching.file_basename_equals("_noplugin_tool2"),
        ],
    )
    action.inputs().not_contains_predicate(matching.file_basename_equals("plugin"))

# Verifies usage of `proto_common.compile` with `additional_inputs` parameter.
def proto_common_compile_additional_inputs(name):
    compile_rule(
        name = "compile_additional_inputs",
        proto_dep = ":simple_proto",
        additional_inputs = ["input1.txt", "input2.txt"],
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_additional_inputs,
        target = ":compile_additional_inputs",
    )

def _proto_common_compile_additional_inputs(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("input1.txt"),
            matching.file_basename_equals("input2.txt"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_tools` parameter and no plugin on the toolchain.
def proto_common_compile_resource_set(name):
    compile_rule(
        name = "compile_resource_set",
        proto_dep = ":simple_proto",
        use_resource_set = True,
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _proto_common_compile_resource_set,
        target = ":compile_resource_set",
    )

def _proto_common_compile_resource_set(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")  # @unused
    # We can't check the specification of the resource set, but we at least verify analysis passes

# Verifies `--protocopts` are passed to command line.
def proto_common_compile_protoc_opts(name):
    compile_rule(
        name = "compile_protoc_opts",
        proto_dep = ":simple_proto",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        target = ":compile_protoc_opts",
        impl = _proto_common_compile_protoc_opts,
        config_settings = {"//command_line_option:protocopt": ["--foo", "--bar"]},
    )

def _proto_common_compile_protoc_opts(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.equals_wrapper("--foo"),
            matching.equals_wrapper("--bar"),
            matching.str_matches("--plugin=b*-out/*-exec-*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

#  Verifies `proto_common.compile`> correctly handles direct generated `.proto` files.
def proto_common_compile_direct_generated_protos(name):
    native.genrule(name = "_generate_G", cmd = "", outs = ["G.proto"], **PREVENT_IMPLICIT_BUILDING)
    proto_library(
        name = "_directly_generated_proto",
        srcs = ["A.proto", "G.proto"],
        **PREVENT_IMPLICIT_BUILDING
    )
    compile_rule(
        name = "compile_directly_generated",
        proto_dep = "_directly_generated_proto",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        target = ":compile_directly_generated",
        impl = _proto_common_compile_direct_generated_protos,
    )

def _proto_common_compile_direct_generated_protos(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.str_matches("--plugin=b*-out/*-exec-*/bin/*/testdata/plugin"),
            matching.str_matches("-Ib*-out/*/genfiles"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
            matching.str_matches("*-out/*/genfiles/*/G.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `plugin_output` parameter
def proto_common_compile_indirect_generated_protos(name):
    native.genrule(name = "_generate_h", srcs = ["A.txt"], cmd = "", outs = ["H.proto"], **PREVENT_IMPLICIT_BUILDING)
    proto_library(name = "_generated_proto", srcs = ["H.proto"], **PREVENT_IMPLICIT_BUILDING)
    proto_library(
        name = "_indirectly_generated_proto",
        srcs = ["A.proto"],
        deps = [":_generated_proto"],
        **PREVENT_IMPLICIT_BUILDING
    )
    compile_rule(
        name = "compile_indirectly_generated",
        proto_dep = "_indirectly_generated_proto",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        target = ":compile_indirectly_generated",
        impl = _proto_common_compile_indirect_generated_protos,
    )

def _proto_common_compile_indirect_generated_protos(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.str_matches("--plugin=b*-out/*-exec-*/bin/*/testdata/plugin"),
            matching.str_matches("-Ib*-out/*/*"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )
