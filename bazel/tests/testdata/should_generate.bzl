# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Testing function for proto_common module"""

load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")

BoolInfo = provider("Simple providers for testing", fields = ["value"])

def _impl(ctx):
    result = proto_common.experimental_should_generate_code(
        ctx.attr.proto_dep[ProtoInfo],
        ctx.attr.toolchain[ProtoLangToolchainInfo],
        "MyRule",
        ctx.attr.proto_dep.label,
    )
    return [BoolInfo(value = result)]

should_generate_rule = rule(
    _impl,
    attrs = {
        "proto_dep": attr.label(),
        "toolchain": attr.label(default = ":toolchain"),
    },
)
