# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Test rule for `proto_common.declare_generated_files`."""

load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")

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

declare_generated_files = rule(
    _impl,
    attrs = {
        "proto_dep": attr.label(),
        "extension": attr.string(),
        "python_names": attr.bool(default = False),
    },
)
