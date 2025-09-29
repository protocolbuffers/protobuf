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

def _impl(ctx):
    files = proto_common.declare_generated_files(
        ctx.actions,
        ctx.attr.proto_dep[ProtoInfo],
        ctx.attr.extension,
        (lambda s: s.replace("-", "_").replace(".", "/")) if ctx.attr.python_names else None,
    )
    for f in files:
        ctx.actions.write(f, "")
    return [DefaultInfo(files = depset(files))]

_declare_generated_files = rule(
    _impl,
    attrs = {
        "proto_dep": attr.label(),
        "extension": attr.string(),
        "python_names": attr.bool(default = False),
    },
)

# Verifies `proto_common.declare_generated_files` call.
def _test_declare_generated_files_basic(name):
    util.helper_target(
        proto_library,
        name = name + "_proto",
        srcs = ["A.proto", "b/B.proto"],
    )
    util.helper_target(
        _declare_generated_files,
        name = name + "_declare_generated_files",
        proto_dep = name + "_proto",
        extension = ".cc",
    )

    analysis_test(
        name = name,
        target = name + "_declare_generated_files",
        impl = _test_declare_generated_files_basic_impl,
    )

def _test_declare_generated_files_basic_impl(env, target):
    env.expect.that_target(target).default_outputs().contains_exactly([
        "{package}/A.cc",
        "{package}/b/B.cc",
    ])

# Verifies `proto_common.declare_generated_files` for Python.
def _test_declare_generated_files_python(name):
    util.helper_target(
        proto_library,
        name = name + "_proto",
        srcs = ["my-proto.gen.proto"],
    )
    util.helper_target(
        _declare_generated_files,
        name = name + "_declare_generated_files_python",
        proto_dep = name + "_proto",
        extension = "_pb2.py",
        python_names = True,
    )

    analysis_test(
        name = name,
        target = name + "_declare_generated_files_python",
        impl = _test_declare_generated_files_python_impl,
    )

def _test_declare_generated_files_python_impl(env, target):
    env.expect.that_target(target).default_outputs().contains_exactly([
        "{package}/my_proto/gen_pb2.py",
    ])
