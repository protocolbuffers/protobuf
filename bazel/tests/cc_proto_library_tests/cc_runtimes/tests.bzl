# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Tests that cc_proto_library depends on the C++ runtime libraries."""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_testing//lib:util.bzl", "TestingAspectInfo")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load("//bazel/tests/cc_proto_library_tests:test_utils.bzl", "CC_PROTO_TESTING_ASPECT")

_HAS_CC_RUNTIMES_TOOLCHAIN_TYPE = bazel_features.cc.cc_runtimes_toolchain_type

# The google3 implementation of cc_proto_library doesn't pass the runtime copts to the compile
# action.
_PASSES_CC_RUNTIMES_COPTS = True

RUNTIME_DEFINE = "PROTOBUF_TEST_MOCK_CC_RUNTIME"
RUNTIME_COPT = "-DPROTOBUF_TEST_MOCK_CC_RUNTIMES_COPT"
RUNTIME_LINKOPT = "-Wl,--protobuf-test-mock-cc-runtime-linkopt"

def _mock_cc_runtime_library_impl(ctx):
    return [CcInfo(
        compilation_context = cc_common.create_compilation_context(
            defines = depset(ctx.attr.defines),
        ),
        linking_context = cc_common.create_linking_context(
            linker_inputs = depset([cc_common.create_linker_input(
                owner = ctx.label,
                user_link_flags = depset(ctx.attr.linkopts),
            )]),
        ),
    )]

# A C++ library that, unlike cc_library, doesn't depend on the C++ runtimes itself.
mock_cc_runtime_library = rule(
    implementation = _mock_cc_runtime_library_impl,
    attrs = {
        "defines": attr.string_list(),
        "linkopts": attr.string_list(),
    },
    provides = [CcInfo],
)

def _mock_cc_runtimes_toolchain_impl(ctx):
    return [platform_common.ToolchainInfo(
        cc_runtimes_info = struct(
            runtimes = ctx.attr.runtimes,
            copts = ctx.attr.copts,
        ),
    )]

mock_cc_runtimes_toolchain = rule(
    implementation = _mock_cc_runtimes_toolchain_impl,
    attrs = {
        "runtimes": attr.label_list(providers = [CcInfo]),
        "copts": attr.string_list(),
    },
)

def _test_cc_runtimes_in_compilation_context(env, target):
    env.expect.that_collection(
        target[CcInfo].compilation_context.defines.to_list(),
    ).contains(RUNTIME_DEFINE)

def _linker_input_flags(target):
    return [
        flag
        for linker_input in target[CcInfo].linking_context.linker_inputs.to_list()
        for flag in linker_input.user_link_flags
    ]

def _test_cc_runtimes_in_linking_context(env, target):
    env.expect.that_collection(_linker_input_flags(target)).contains(RUNTIME_LINKOPT)

def _test_cc_runtimes_copts_in_compile_action(env, target):
    compile_actions = [
        a
        for a in target[TestingAspectInfo].actions
        if a.mnemonic == "CppCompile"
    ]
    env.expect.that_bool(len(compile_actions) > 0).equals(True)
    for action in compile_actions:
        env.expect.that_collection(action.argv).contains(RUNTIME_COPT)

def _test_no_cc_runtimes_without_toolchain(env, target):
    env.expect.that_collection(
        target[CcInfo].compilation_context.defines.to_list(),
    ).not_contains(RUNTIME_DEFINE)

def _test_no_cc_runtimes_in_linking_context_without_toolchain(env, target):
    env.expect.that_collection(_linker_input_flags(target)).not_contains(RUNTIME_LINKOPT)

def cc_runtimes_test_suites(name, toolchain, no_runtime_cc_proto_toolchain):
    """Defines the cc_runtimes test suites.

    Args:
      name: Prefix for the test suite names.
      toolchain: Label of the toolchain() target that provides the mock C++ runtimes.
      no_runtime_cc_proto_toolchain: Label of a proto_lang_toolchain() for C++ without a runtime
        library. Its toolchain() target must be named `<no_runtime_cc_proto_toolchain>_toolchain`.
    """

    # Without the mock toolchain, cc_proto_library works and doesn't depend on the mock runtime.
    # This also covers Bazel versions without the cc_runtimes toolchain type.
    bzl_test_suite(
        name = name + "_without_toolchain",
        tests = {
            ":foo_cc_proto": [
                _test_no_cc_runtimes_without_toolchain,
                _test_no_cc_runtimes_in_linking_context_without_toolchain,
            ],
        },
    )

    if not _HAS_CC_RUNTIMES_TOOLCHAIN_TYPE:
        native.test_suite(
            name = name,
            tests = [name + "_without_toolchain"],
        )
        return

    config_settings = {
        "//command_line_option:extra_toolchains": [str(native.package_relative_label(toolchain))],
    }
    bzl_test_suite(
        name = name + "_with_toolchain",
        config_settings = config_settings,
        tests = {
            ":foo_cc_proto": [
                _test_cc_runtimes_in_compilation_context,
                _test_cc_runtimes_in_linking_context,
            ],
        },
    )

    # The proto runtime is normally a cc_library, which depends on the C++ runtimes itself, so the
    # tests above would pass even if cc_proto_library didn't depend on them directly. Hence, also
    # test with a proto runtime that doesn't depend on the C++ runtimes:
    # - in google3, `:foo_no_proto_runtime_proto` sets `cc_lib` to an empty library.
    # - in OSS, the C++ proto toolchain is replaced with one that doesn't have a runtime.
    no_runtime_toolchain_label = native.package_relative_label(no_runtime_cc_proto_toolchain)
    bzl_test_suite(
        name = name + "_with_toolchain_no_proto_runtime",
        config_settings = {
            "//command_line_option:extra_toolchains": [
                str(native.package_relative_label(toolchain)),
                str(no_runtime_toolchain_label.same_package_label(no_runtime_toolchain_label.name + "_toolchain")),
            ],
            str(Label("//bazel/flags/cc:proto_toolchain_for_cc")): str(no_runtime_toolchain_label),
        },
        tests = {
            ":foo_no_proto_runtime_cc_proto": [
                _test_cc_runtimes_in_compilation_context,
                _test_cc_runtimes_in_linking_context,
            ],
        },
    )

    suites = [
        name + "_with_toolchain",
        name + "_with_toolchain_no_proto_runtime",
        name + "_without_toolchain",
    ]

    if _PASSES_CC_RUNTIMES_COPTS:
        bzl_test_suite(
            name = name + "_with_toolchain_aspect",
            config_settings = config_settings,
            testing_aspect = CC_PROTO_TESTING_ASPECT,
            tests = {
                ":foo_proto": [_test_cc_runtimes_copts_in_compile_action],
            },
        )
        suites.append(name + "_with_toolchain_aspect")

    native.test_suite(
        name = name,
        tests = suites,
    )
