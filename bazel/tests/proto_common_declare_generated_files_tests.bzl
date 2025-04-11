# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Tests for `proto_common.declare_generated_files` function."""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test", "test_suite")
load("@rules_testing//lib:util.bzl", "util")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")

def proto_common_declare_generated_files_test_suite(name):
    test_suite(
        name = name,
        tests = [
            _test_declare_generated_files_basic,
            _test_declare_generated_files_python,
        ],
    )

# Verifies `proto_common.declare_generated_files` call.
def _test_declare_generated_files_basic(name):
    util.helper_target(
        proto_library,
        name = name + "_proto",
        srcs = ["A.proto", "b/B.proto"],
    )

    analysis_test(
        name = name,
        target = name + "_proto",
        impl = _test_declare_generated_files_basic_impl,
    )

def _test_declare_generated_files_basic_impl(env, target):
    files = proto_common.declare_generated_files(
        env.ctx.actions,
        target[ProtoInfo],
        extension = ".cc",
    )
    env.expect.that_depset_of_files(files).contains_exactly([
        target.label.package + "/A.cc",
        target.label.package + "/b/B.cc",
    ])

    # Attempted workaround: create declared files to prevent failure of the test framework
    [env.ctx.actions.write(f, "") for f in files]
    # This doesn't work, because we don't allow actions in the assert functions

# Verifies `proto_common.declare_generated_files` for Python.
def _test_declare_generated_files_python(name):
    util.helper_target(
        proto_library,
        name = name + "_proto",
        srcs = ["my-proto.gen.proto"],
    )

    analysis_test(
        name = name,
        target = name + "_proto",
        impl = _test_declare_generated_files_python_impl,
    )

def _test_declare_generated_files_python_impl(env, target):
    files = proto_common.declare_generated_files(
        env.ctx.actions,
        target[ProtoInfo],
        extension = "_pb2.py",
        name_mapper = lambda s: s.replace("-", "_").replace(".", "/"),
    )
    env.expect.that_depset_of_files(files).contains_exactly([
        target.label.package + "/my_proto/gen_pb2.py",
    ])

    # Attempted workaround: create declared files to prevent failure of the test framework
    [env.ctx.actions.write(f, "") for f in files]
    # This doesn't work, because we don't allow actions in the assert functions
