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

def _resource_set_callback(_os, inputs_size):
    return {"memory": 25 + 0.15 * inputs_size, "cpu": 1}

def _impl(ctx):
    outfile = ctx.actions.declare_file(ctx.attr.name)
    kwargs = {}
    if ctx.attr.plugin_output == "single":
        kwargs["plugin_output"] = outfile.path
    elif ctx.attr.plugin_output == "multiple":
        kwargs["plugin_output"] = ctx.bin_dir.path
    elif ctx.attr.plugin_output == "wrong":
        kwargs["plugin_output"] = ctx.bin_dir.path + "///"
    if ctx.attr.additional_args:
        additional_args = ctx.actions.args()
        additional_args.add_all(ctx.attr.additional_args)
        kwargs["additional_args"] = additional_args
    if ctx.files.additional_tools:
        kwargs["additional_tools"] = ctx.files.additional_tools
    if ctx.files.additional_inputs:
        kwargs["additional_inputs"] = depset(ctx.files.additional_inputs)
    if ctx.attr.use_resource_set:
        kwargs["resource_set"] = _resource_set_callback
    if ctx.attr.progress_message:
        kwargs["experimental_progress_message"] = ctx.attr.progress_message
    proto_common.compile(
        ctx.actions,
        ctx.attr.proto_dep[ProtoInfo],
        ctx.attr.toolchain[proto_common.ProtoLangToolchainInfo],
        [outfile],
        **kwargs
    )
    return [DefaultInfo(files = depset([outfile]))]

compile_rule = rule(
    _impl,
    attrs = {
        "proto_dep": attr.label(),
        "plugin_output": attr.string(),
        "toolchain": attr.label(default = ":toolchain"),
        "additional_args": attr.string_list(),
        "additional_tools": attr.label_list(cfg = "exec"),
        "additional_inputs": attr.label_list(allow_files = True),
        "use_resource_set": attr.bool(),
        "progress_message": attr.string(),
    },
)
