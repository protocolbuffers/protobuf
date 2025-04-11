# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Test rule for `proto_common.check_collocated`."""

load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")

def _impl(ctx):
    proto_common.check_collocated(
        ctx.label,
        ctx.attr.proto_dep[ProtoInfo],
        ctx.attr.toolchain[ProtoLangToolchainInfo],
    )
    return None

check_collocated = rule(
    _impl,
    attrs = {
        "proto_dep": attr.label(),
        "toolchain": attr.label(default = ":toolchain"),
    },
)
