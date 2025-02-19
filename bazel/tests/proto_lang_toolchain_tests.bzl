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
load("//tools/build_defs/unittest:unittest.bzl", "asserts")

def proto_lang_toolchain_test_suite(name):
    util.helper_target(
        proto_library,
        name = "simple2_proto",
        srcs = ["A.proto"],
    )
    test_suite(
        name = name,
        tests = [
            _test_proto_toolchain,
        ],
    )

def _test_proto_toolchain(name):
    analysis_test(
        name = name,
        target = ":toolchain",
        impl = _test_proto_lang_toolchain_impl,
    )

def _test_proto_lang_toolchain_impl(env, target):
    action = env.expect.that_target(target).action_named("MyMnemonic")

    # Validate proto_lang_toolchain
    asserts.equals(env, "cmd-line:%s", target.out_replacement_format_flag)
    asserts.equals(env, "--plugin=%s", target.plugin_format_flag)
    asserts.equals(env, "third_party/x/plugin", target.plugin_executable.executable.short_path)
    asserts.equals(env, "//third_party/x:runtime", str(target.runtime.label))
    asserts.equals(env, ["--myflag"], target.protoc_opts)
    asserts.equals(env, "Progress Message %{label}", target.progress_message)
    asserts.equals(env, "MyMnemonic", target.mnemonic)

    # Validate proto_compiler
    # TODO: Add tests for proto_compiler.
