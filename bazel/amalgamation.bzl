# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
