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
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")

# Test suite for `proto_lang_toolchain` rule.
def proto_lang_toolchain_test_suite(name):
    test_suite(
        name = name,
        tests = [
            _test_proto_toolchain,
            _test_optional_fields_are_empty,
        ],
    )

def _test_proto_toolchain(name):
    analysis_test(
        name = name,
        target = "//bazel/tests/testdata:toolchain",
        impl = _test_proto_lang_toolchain_impl,
        config_settings = {"//command_line_option:protocopt": ["--myflag"]},
    )

def _test_proto_lang_toolchain_impl(env, target):
    proto_lang_toolchain_info = env.expect.that_target(target).actual[ProtoLangToolchainInfo]

    # Validate proto_lang_toolchain
    env.expect.that_str(proto_lang_toolchain_info.out_replacement_format_flag).equals("--java_out=param1,param2:%s")
    env.expect.that_str(proto_lang_toolchain_info.plugin_format_flag).equals("--plugin=%s")
    env.expect.that_str(proto_lang_toolchain_info.plugin.executable.short_path).contains("bazel/tests/testdata/plugin")

    env.expect.that_str(str(proto_lang_toolchain_info.runtime.label)).equals("//bazel/tests/testdata:runtime")

    env.expect.that_str(proto_lang_toolchain_info.progress_message).equals("Progress Message %{label}")
    env.expect.that_str(proto_lang_toolchain_info.mnemonic).equals("MyMnemonic")

    env.expect.that_collection(proto_lang_toolchain_info.protoc_opts).contains_exactly(["--myflag"])

    # Assert that toolchain's blacklist contains the expected protos from testdata:toolchain
    # The testdata:toolchain has blacklisted_protos = [":denied"], which contains 3 protos.
    env.expect.that_collection(proto_lang_toolchain_info.provided_proto_sources).has_size(3)
    env.expect.that_collection(proto_lang_toolchain_info.provided_proto_sources).contains_at_least_predicates([
        matching.file_basename_equals("any.proto"),
        matching.file_basename_equals("descriptor.proto"),
        matching.file_basename_equals("metadata.proto"),
    ])

    # Validate proto_compiler
    protoc_suffix = "protoc"
    env.expect.that_str(str(proto_lang_toolchain_info.proto_compiler.executable.short_path)).contains(protoc_suffix)

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

    # Assert that toolchain's blacklist is empty for this test case
    env.expect.that_collection(proto_lang_toolchain_info.provided_proto_sources).has_size(0)
