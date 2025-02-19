# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `proto_lang_toolchain` function."""

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")

# Test suite for `proto_lang_toolchain` rule.
def proto_lang_toolchain_test_suite(name):
    util.helper_target(
        cc_binary,
        name = "plugin",
        srcs = ["plugin.cc"],
    )

    util.helper_target(
        cc_library,
        name = "runtime",
        srcs = ["runtime.cc"],
    )

    util.helper_target(
        native.filegroup,
        name = "descriptors",
        srcs = [
            "descriptor.proto",
            "metadata.proto",
        ],
    )

    util.helper_target(
        proto_lang_toolchain,
        name = "toolchain",
        command_line = "cmd-line:%s",
        plugin_format_flag = "--plugin=%s",
        plugin = ":plugin",
        runtime = ":runtime",
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

def _test_proto_toolchain_resolution_enabled(name):
    util.helper_target(
        native.filegroup,
        name = "any",
        srcs = ["any.proto"],
    )

    analysis_test(
        name = name,
        target = ":toolchain",
        impl = _test_proto_lang_toolchain_impl,
    )

# Verifies `proto_lang_toolchain` call with proto_library in denylist.
def _test_proto_toolchain_denylist_proto_libraries(name):
    util.helper_target(
        proto_library,
        name = name + "any",
        srcs = [
            "any.proto",
        ],
        strip_import_prefix = "/third_party",
    )

    analysis_test(
        name = name,
        target = ":toolchain",
        impl = _test_proto_lang_toolchain_impl,
    )

# Verifies `proto_lang_toolchain` call with transitive proto_library in denylist.
def _test_proto_toolchain_denylist_transitive_protos(name):
    util.helper_target(
        proto_library,
        name = name + "any",
        srcs = [
            "any.proto",
        ],
        deps = [
            ":descriptors",
        ],
    )

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
    env.expect.that_str(proto_lang_toolchain_info.plugin.executable.short_path).equals("third_party/protobuf/bazel/tests/plugin")
    env.expect.that_str(str(proto_lang_toolchain_info.runtime.label)).equals("//bazel/tests:runtime")
    env.expect.that_str(proto_lang_toolchain_info.progress_message).equals("Progress Message %{label}")
    env.expect.that_str(proto_lang_toolchain_info.mnemonic).equals("MyMnemonic")

    # Validate proto_compiler
    actual_protoc_label = "net/proto2/compiler/public/protocol_compiler"
    env.expect.that_str(str(proto_lang_toolchain_info.proto_compiler.executable.short_path)).equals(actual_protoc_label)

def _test_optional_fields_are_empty(name):
    util.helper_target(
        proto_lang_toolchain,
        name = name + "toolchain",
        command_line = "cmd-line:%s",
    )
    analysis_test(
        name = name,
        target = name + "toolchain",
        impl = _test_optional_fields_are_empty_impl,
    )

# Verifies `proto_lang_toolchain` call with optional fields empty.
def _test_optional_fields_are_empty_impl(env, target):
    proto_lang_toolchain_info = env.expect.that_target(target).actual[ProtoLangToolchainInfo]

    # Validate proto_lang_toolchain
    env.expect.that_str(proto_lang_toolchain_info.plugin).equals(None)
    env.expect.that_str(proto_lang_toolchain_info.runtime).equals(None)
    env.expect.that_str(proto_lang_toolchain_info.mnemonic).equals("GenProto")
