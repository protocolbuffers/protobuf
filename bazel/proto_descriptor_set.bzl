# Copyright (c) 2020-2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""A rule for generating a `FileDescriptorSet` with all transitive dependencies.

This module contains the definition of `proto_descriptor_set`, a rule that
collects all `FileDescriptorSet`s from its transitive dependencies and generates
a single `FileDescriptorSet` containing all the `FileDescriptorProto` from them.
"""

load("//bazel/common:proto_info.bzl", "ProtoInfo")

def _proto_descriptor_set_impl(ctx):
    args = ctx.actions.args()

    output = ctx.actions.declare_file("{}.pb".format(ctx.attr.name))
    args.add(output)

    descriptor_sets = depset(
        transitive = [dep[ProtoInfo].transitive_descriptor_sets for dep in ctx.attr.deps],
    )
    args.add_all(descriptor_sets)

    ctx.actions.run(
        executable = ctx.executable._file_concat,
        mnemonic = "ConcatFileDescriptorSet",
        inputs = descriptor_sets,
        outputs = [output],
        arguments = [args],
    )

    return [
        DefaultInfo(
            files = depset([output]),
            runfiles = ctx.runfiles(files = [output]),
        ),
    ]

proto_descriptor_set = rule(
    implementation = _proto_descriptor_set_impl,
    attrs = {
        "deps": attr.label_list(
            mandatory = False,
            providers = [ProtoInfo],
            doc = """
Sequence of `ProtoInfo`s to collect `FileDescriptorSet`s from.
""".strip(),
        ),
        "_file_concat": attr.label(
            default = "//bazel/private/file_concat:file_concat",
            executable = True,
            cfg = "exec",
        ),
    },
    doc = """
Collects all `FileDescriptorSet`s from `deps` and combines them into a single
`FileDescriptorSet` containing all the `FileDescriptorProto`.
""".strip(),
)
