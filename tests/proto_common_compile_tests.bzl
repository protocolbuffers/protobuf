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
            _compile_basic_test,
            _compile_noplugin_test,
            _compile_with_plugin_output_test,
            _compile_with_directory_plugin_output_test,
            _compile_additional_args_test,
            _compile_additional_tools_test,
            _compile_additional_tools_no_plugin_test,
            _compile_additional_inputs_test,
            _compile_resource_set_test,
            _compile_protoc_opts_test,
            _compile_direct_generated_protos_test,
            _compile_indirect_generated_protos_test,
        ],
    )

# Verifies basic usage of `proto_common.compile`.
def _compile_basic_test(name):
    compile_rule(
        name = "_compile_basic_test",
        proto_dep = ":simple_proto",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_basic,
        target = ":_compile_basic_test",
    )

def _compile_basic(env, target):
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
def _compile_noplugin_test(name):
    compile_rule(
        name = "_compile_noplugin_test",
        proto_dep = ":simple_proto",
        toolchain = "//src/google/protobuf/github/tests/testdata:toolchain_noplugin",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_noplugin,
        target = ":_compile_noplugin_test",
    )

def _compile_noplugin(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `plugin_output` parameter set to file.
def _compile_with_plugin_output_test(name):
    compile_rule(
        name = "_compile_with_plugin_output_test",
        proto_dep = ":simple_proto",
        plugin_output = "single",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_with_plugin_output,
        target = ":_compile_with_plugin_output_test",
    )

def _compile_with_plugin_output(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.argv().contains_exactly_predicates(
        [
            matching.str_endswith("/protocol_compiler"),
            matching.str_matches("--java_out=param1,param2:b*-out/*/_compile_with_plugin_output_test"),
            matching.str_matches("--plugin=b*-out/*-exec-*/bin/*/testdata/plugin"),
            matching.equals_wrapper("-I."),
            matching.str_endswith("/A.proto"),
        ],
    )

# Verifies usage of `proto_common.compile` with `plugin_output` parameter set to file.
def _compile_with_directory_plugin_output_test(name):
    compile_rule(
        name = "_compile_with_directory_plugin_output_test",
        proto_dep = ":simple_proto",
        plugin_output = "multiple",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_with_directory_plugin_output,
        target = ":_compile_with_directory_plugin_output_test",
    )

def _compile_with_directory_plugin_output(env, target):
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
def _compile_additional_args_test(name):
    compile_rule(
        name = "_compile_additional_args_test",
        proto_dep = ":simple_proto",
        additional_args = ["--a", "--b"],
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_additional_args,
        target = ":_compile_additional_args_test",
    )

def _compile_additional_args(env, target):
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
def _compile_additional_tools_test(name):
    compile_rule(
        name = "_compile_additional_tools_test",
        proto_dep = ":simple_proto",
        additional_tools = [
            "//src/google/protobuf/github/tests/testdata:_tool1",
            "//src/google/protobuf/github/tests/testdata:_tool2",
        ],
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_additional_tools,
        target = ":_compile_additional_tools_test",
    )

def _compile_additional_tools(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("_tool1"),
            matching.file_basename_equals("_tool2"),
            matching.file_basename_equals("plugin"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_tools` parameter and no plugin on the toolchain.
def _compile_additional_tools_no_plugin_test(name):
    compile_rule(
        name = "compile_additional_tools_noplugin",
        proto_dep = ":simple_proto",
        additional_tools = [
            "//src/google/protobuf/github/tests/testdata:_tool1",
            "//src/google/protobuf/github/tests/testdata:_tool2",
        ],
        toolchain = "//src/google/protobuf/github/tests/testdata:toolchain_noplugin",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_additional_tools_no_plugin,
        target = ":compile_additional_tools_noplugin",
    )

def _compile_additional_tools_no_plugin(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("_tool1"),
            matching.file_basename_equals("_tool2"),
        ],
    )
    action.inputs().not_contains_predicate(matching.file_basename_equals("plugin"))

# Verifies usage of `proto_common.compile` with `additional_inputs` parameter.
def _compile_additional_inputs_test(name):
    compile_rule(
        name = "compile_additional_inputs",
        proto_dep = ":simple_proto",
        additional_inputs = ["input1.txt", "input2.txt"],
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_additional_inputs,
        target = ":compile_additional_inputs",
    )

def _compile_additional_inputs(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")
    action.inputs().contains_at_least_predicates(
        [
            matching.file_basename_equals("input1.txt"),
            matching.file_basename_equals("input2.txt"),
        ],
    )

# Verifies usage of `proto_common.compile` with `additional_tools` parameter and no plugin on the toolchain.
def _compile_resource_set_test(name):
    compile_rule(
        name = "compile_resource_set",
        proto_dep = ":simple_proto",
        use_resource_set = True,
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        impl = _compile_resource_set,
        target = ":compile_resource_set",
    )

def _compile_resource_set(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")  # @unused
    # We can't check the specification of the resource set, but we at least verify analysis passes

# Verifies `--protocopts` are passed to command line.
def _compile_protoc_opts_test(name):
    compile_rule(
        name = "compile_protoc_opts",
        proto_dep = ":simple_proto",
        **PREVENT_IMPLICIT_BUILDING
    )

    analysis_test(
        name = name,
        target = ":compile_protoc_opts",
        impl = _compile_protoc_opts,
        config_settings = {"//command_line_option:protocopt": ["--foo", "--bar"]},
    )

def _compile_protoc_opts(env, target):
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
def _compile_direct_generated_protos_test(name):
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
        impl = _compile_direct_generated_protos,
    )

def _compile_direct_generated_protos(env, target):
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
def _compile_indirect_generated_protos_test(name):
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
        impl = _compile_indirect_generated_protos,
    )

def _compile_indirect_generated_protos(env, target):
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
