# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Internal rules for building upb."""

load(":upb_proto_library.bzl", "GeneratedSrcsInfo")

# upb_amalgamation() rule, with file_list aspect.

SrcList = provider(
    fields = {
        "srcs": "list of srcs",
    },
)

def _file_list_aspect_impl(target, ctx):
    if GeneratedSrcsInfo in target:
        srcs = target[GeneratedSrcsInfo]
        return [SrcList(srcs = srcs.srcs + srcs.hdrs)]

    srcs = []
    for src in ctx.rule.attr.srcs:
        srcs += src.files.to_list()
    for hdr in ctx.rule.attr.hdrs:
        srcs += hdr.files.to_list()
    for hdr in ctx.rule.attr.textual_hdrs:
        srcs += hdr.files.to_list()
    return [SrcList(srcs = srcs)]

_file_list_aspect = aspect(
    implementation = _file_list_aspect_impl,
)

def _upb_amalgamation(ctx):
    inputs = []
    for lib in ctx.attr.libs:
        inputs += lib[SrcList].srcs
    srcs = [src for src in inputs if not src.path.endswith("hpp")]
    ctx.actions.run(
        inputs = inputs,
        outputs = ctx.outputs.outs,
        arguments = [f.path for f in ctx.outputs.outs] + [f.path for f in srcs],
        progress_message = "Making amalgamation",
        executable = ctx.executable._amalgamator,
    )
    return []

upb_amalgamation = rule(
    attrs = {
        "_amalgamator": attr.label(
            executable = True,
            cfg = "exec",
            default = "//bazel:amalgamate",
        ),
        "prefix": attr.string(
            default = "",
        ),
        "libs": attr.label_list(aspects = [_file_list_aspect]),
        "outs": attr.output_list(),
        "strip_import_prefix": attr.string_list(),
    },
    implementation = _upb_amalgamation,
)
