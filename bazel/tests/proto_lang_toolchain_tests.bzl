# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `proto_lang_toolchain` function."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:truth.bzl", "matching")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")

def proto_lang_toolchain_test_suite(name):
    # Are these needed?
    # licenses(["unencumbered"])

    cc_binary(
        name = "plugin",
        srcs = ["plugin.cc"],
    )
    cc_library(
        name = "runtime",
        srcs = ["runtime.cc"],
    )
    # filegroup(
    #     name = "descriptors",
    #     srcs = [
    #         "descriptor.proto",
    #         "metadata.proto",
    #     ],
    # )
    # filegroup(
    #     name = "any",
    #     srcs = ["any.proto"],
    # )

    util.helper_target(
        proto_library,
        name = "denied",
        srcs = [
            ":any",
            ":descriptors",
        ],
    )

    util.helper_target(
        proto_lang_toolchain,
        name = "toolchain",
        out_replacement_format_flag = "cmd-line:%s",
        plugin_format_flag = "--plugin=%s",
        plugin = ":plugin",
        runtime = ":runtime",
        protoc_opts = ["--myflag"],
        progress_message = "Progress Message %{label}",
        mnemonic = "MyMnemonic",
    )
    test_suite(
        name = name,
        tests = [
            _test_proto_toolchain,
            _test_proto_toolchain_resolution_enabled,
            _test_proto_toolchain_denylist_proto_libraries,
            _test_proto_toolchain_denylist_transitive_protos,
            _test_optional_fields_are_empty,
        ],
    )

def _test_proto_toolchain(name):
    analysis_test(
        name = name,
        target = ":toolchain",
        impl = _test_proto_lang_toolchain_impl,
    )

def _test_proto_lang_toolchain_impl(env, target):
    proto_lang_toolchain_info = env.expect.that_target(target).actual[ProtoLangToolchainInfo]

    # Validate proto_lang_toolchain
    env.expect.that_str(proto_lang_toolchain_info.out_replacement_format_flag).equals("cmd-line:%s")
    env.expect.that_str(proto_lang_toolchain_info.plugin_format_flag).equals("--plugin=%s")
    env.expect.that_str(proto_lang_toolchain_info.plugin_executable.executable.short_path).equals("plugin")
    env.expect.that_str(str(proto_lang_toolchain_info.runtime.label)).equals("//:runtime")
    env.expect.that_list(proto_lang_toolchain_info.protoc_opts).equals(["--myflag"])
    env.expect.that_str(proto_lang_toolchain_info.progress_message).equals("Progress Message %{label}")
    env.expect.that_str(proto_lang_toolchain_info.mnemonic).equals("MyMnemonic")

    # Validate proto_compiler
    actual_protoc_label = "@//net/proto2/compiler/public:protocol_compiler"
    env.expect.that_str(str(target.proto_compiler.executable.short_path)).equals(actual_protoc_label)

def _test_proto_toolchain_resolution_enabled(name):
    # licenses(["unencumbered"])

    cc_binary(
        name = "plugin",
        srcs = ["plugin.cc"],
    )

    cc_library(
        name = "runtime",
        srcs = ["runtime.cc"],
    )

    # filegroup(
    #     name = "descriptors",
    #     srcs = [
    #         "descriptor.proto",
    #         "metadata.proto",
    #     ],
    # )

    # filegroup(
    #     name = "any",
    #     srcs = ["any.proto"],
    # )

    util.helper_target(
        proto_library,
        name = "denied",
        srcs = [
            ":any",
            ":descriptors",
        ],
    )

    util.helper_target(
        proto_lang_toolchain,
        name = "toolchain",
        out_replacement_format_flag = "cmd-line:%s",
        plugin_format_flag = "--plugin=%s",
        plugin = ":plugin",
        runtime = ":runtime",
        protoc_opts = ["--myflag"],
        progress_message = "Progress Message %{label}",
        mnemonic = "MyMnemonic",
    )

    analysis_test(
        name = name,
        target = ":toolchain",
        impl = _test_proto_lang_toolchain_impl,
    )

def _test_proto_toolchain_denylist_proto_libraries(name):
    # licenses(['unencumbered'])

    cc_binary(
        name = "plugin",
        srcs = ["plugin.cc"],
    )

    cc_library(
        name = "runtime",
        srcs = ["runtime.cc"],
    )

    util.helper_target(
        proto_library,
        name = "descriptors",
        srcs = [
            "metadata.proto",
            "descriptor.proto",
        ],
    )

    util.helper_target(
        proto_library,
        name = "any",
        srcs = [
            "any.proto",
        ],
        strip_import_prefix = "/third_party",
    )

    util.helper_target(
        proto_lang_toolchain,
        name = "toolchain",
        out_replacement_format_flag = "cmd-line:%s",
        plugin_format_flag = "--plugin=%s",
        plugin = ":plugin",
        runtime = ":runtime",
        protoc_opts = ["--myflag"],
        progress_message = "Progress Message %{label}",
        mnemonic = "MyMnemonic",
    )

    analysis_test(
        name = name,
        target = ":toolchain",
        impl = _test_proto_lang_toolchain_impl,
    )

def _test_proto_toolchain_denylist_transitive_protos(name):
    # licenses(["unencumbered"])

    cc_binary(
        name = "plugin",
        srcs = ["plugin.cc"],
    )

    cc_library(
        name = "runtime",
        srcs = ["runtime.cc"],
    )

    util.helper_target(
        proto_library,
        name = "descriptors",
        srcs = [
            "metadata.proto",
            "descriptor.proto",
        ],
    )

    util.helper_target(
        proto_library,
        name = "any",
        srcs = [
            "any.proto",
        ],
        deps = [
            ":descriptors",
        ],
    )

    util.helper_target(
        proto_lang_toolchain,
        name = "toolchain",
        out_replacement_format_flag = "cmd-line:%s",
        plugin_format_flag = "--plugin=%s",
        plugin = ":plugin",
        runtime = ":runtime",
        protoc_opts = ["--myflag"],
        progress_message = "Progress Message %{label}",
        mnemonic = "MyMnemonic",
    )

    analysis_test(
        name = name,
        target = ":toolchain",
        impl = _test_proto_lang_toolchain_impl,
    )

def _test_optional_fields_are_empty(name):
    # TestConstants.LOAD_PROTO_LANG_TOOLCHAIN
    # public static final String LOAD_PROTO_LANG_TOOLCHAIN =
    #     "load('@//bazel/toolchains:proto_lang_toolchain.bzl',"
    #         + " 'proto_lang_toolchain')";

    util.helper_target(
        proto_lang_toolchain,
        name = "toolchain",
        out_replacement_format_flag = "cmd-line:%s",
    )
    analysis_test(
        name = name,
        target = ":toolchain",
        impl = _test_optional_fields_are_empty_impl,
    )

def _test_optional_fields_are_empty_impl(env, target):
    proto_lang_toolchain_info = env.expect.that_target(target).actual[ProtoLangToolchainInfo]

    # Validate proto_lang_toolchain
    env.expect.that_str(proto_lang_toolchain_info.plugin_executable.executable.short_path).is_none()
    env.expect.that_str(proto_lang_toolchain_info.runtime.label).is_none()
    env.expect.that_str(proto_lang_toolchain_info.mnemonic).equals("GenProto")
