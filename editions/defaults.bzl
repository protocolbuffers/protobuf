# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""
Provide a rule for generating the intermediate feature set defaults used for feature resolution.

See go/life-of-a-featureset for more information.
"""

load("//bazel/common:proto_info.bzl", "ProtoInfo")

def _compile_edition_defaults_impl(ctx):
    out_file = ctx.actions.declare_file(ctx.outputs.output.basename)
    sources = []
    paths = []
    for src in ctx.attr.srcs:
        sources.extend(src[ProtoInfo].transitive_sources.to_list())
        paths.extend(src[ProtoInfo].transitive_proto_path.to_list())

    args = ctx.actions.args()
    args.add("--edition_defaults_out", out_file)

    args.add("--edition_defaults_minimum", ctx.attr.minimum_edition)
    args.add("--edition_defaults_maximum", ctx.attr.maximum_edition)
    for p in paths:
        args.add("--proto_path", p)
    for source in sources:
        args.add(source)
    ctx.actions.run(
        outputs = [out_file],
        inputs = sources,
        executable = ctx.executable.protoc or ctx.executable._protoc_minimal,
        arguments = [args],
        progress_message = "Generating edition defaults",
    )

compile_edition_defaults = rule(
    attrs = {
        "srcs": attr.label_list(
            mandatory = True,
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
        "minimum_edition": attr.string(mandatory = True),
        "maximum_edition": attr.string(mandatory = True),
        "protoc": attr.label(
            mandatory = False,
            executable = True,
            cfg = "exec",
        ),
        "_protoc_minimal": attr.label(
            default = "//src/google/protobuf/compiler:protoc_minimal",
            executable = True,
            cfg = "exec",
        ),
    },
    implementation = _compile_edition_defaults_impl,
    outputs = {
        "output": "%{name}.binpb",
    },
)

def _embed_edition_defaults_impl(ctx):
    args = ctx.actions.args()
    args.add(ctx.attr.encoding, format = "--encoding=%s")
    args.add(ctx.file.defaults, format = "--defaults_path=%s")
    args.add(ctx.file.template, format = "--template_path=%s")
    args.add(ctx.outputs.output, format = "--output_path=%s")
    args.add(ctx.attr.placeholder, format = "--placeholder=%s")

    ctx.actions.run(
        executable = ctx.executable._escape,
        arguments = [args],
        outputs = [ctx.outputs.output],
        inputs = [ctx.file.defaults, ctx.file.template],
    )

embed_edition_defaults = rule(
    doc = "genrule to embed edition defaults binary data into a template file.",
    attrs = {
        "defaults": attr.label(
            mandatory = True,
            allow_single_file = True,
            allow_rules = ["compile_edition_defaults"],
            providers = [ProtoInfo],
            doc = "The compile_edition_defaults rule to embed",
        ),
        "output": attr.output(
            mandatory = True,
            doc = "The name of the output file",
        ),
        "template": attr.label(
            mandatory = True,
            allow_single_file = True,
            doc = "The template to use for generating the output file",
        ),
        "placeholder": attr.string(
            mandatory = True,
            doc = "The placeholder to replace with a serialized string in the template",
        ),
        "encoding": attr.string(
            default = "octal",
            values = ["octal", "base64"],
            doc = "The encoding format to use for the binary data (octal or base64)",
        ),
        "_escape": attr.label(
            default = "//editions:internal_defaults_escape",
            executable = True,
            cfg = "exec",
        ),
    },
    implementation = _embed_edition_defaults_impl,
)
