# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""An implementation of py_proto_library().

We have to implement this ourselves because there is currently no reasonable
py_proto_library() rule available for Bazel.

Our py_proto_library() is similar to how a real py_proto_library() should work.
But it hasn't been deeply tested or reviewed, and upb should not be in the
business of vending py_proto_library(), so we keep it private to upb.
"""

load("@bazel_skylib//lib:paths.bzl", "paths")

# begin:github_only
load("@rules_proto//proto:defs.bzl", "ProtoInfo")
# end:github_only

# Generic support code #########################################################

# begin:github_only
_is_google3 = False
# end:github_only

# begin:google_only
# _is_google3 = True
# end:google_only

def _get_real_short_path(file):
    # For some reason, files from other archives have short paths that look like:
    #   ../com_google_protobuf/google/protobuf/descriptor.proto
    short_path = file.short_path
    if short_path.startswith("../"):
        second_slash = short_path.index("/", 3)
        short_path = short_path[second_slash + 1:]

    # Sometimes it has another few prefixes like:
    #   _virtual_imports/any_proto/google/protobuf/any.proto
    #   benchmarks/_virtual_imports/100_msgs_proto/benchmarks/100_msgs.proto
    # We want just google/protobuf/any.proto.
    virtual_imports = "_virtual_imports/"
    if virtual_imports in short_path:
        short_path = short_path.split(virtual_imports)[1].split("/", 1)[1]
    return short_path

def _get_real_root(ctx, file):
    real_short_path = _get_real_short_path(file)
    root = file.path[:-len(real_short_path) - 1]

    if not _is_google3 and ctx.rule.attr.strip_import_prefix:
        root = paths.join(root, ctx.rule.attr.strip_import_prefix[1:])
    return root

def _generate_output_file(ctx, src, extension):
    package = ctx.label.package
    if not _is_google3:
        strip_import_prefix = ctx.rule.attr.strip_import_prefix
        if strip_import_prefix and strip_import_prefix != "/":
            if not package.startswith(strip_import_prefix[1:]):
                fail("%s does not begin with prefix %s" % (package, strip_import_prefix))
            package = package[len(strip_import_prefix):]

    real_short_path = _get_real_short_path(src)
    real_short_path = paths.relativize(real_short_path, package)
    output_filename = paths.replace_extension(real_short_path, extension)
    ret = ctx.actions.declare_file(output_filename)
    return ret

# py_proto_library() ###########################################################

def _py_proto_library_rule_impl(ctx):
    # A real py_proto_library() should enforce this constraint.
    # We don't bother for now, since it saves us some effort not to.
    #
    # if len(ctx.attr.deps) != 1:
    #     fail("only one deps dependency allowed.")

    files = []
    for dep in ctx.attr.deps:
        files += dep[PyInfo].transitive_sources.to_list()
    return [
        DefaultInfo(files = depset(direct = files)),
    ]

def _py_proto_library_aspect_impl(target, ctx):
    proto_info = target[ProtoInfo]
    proto_sources = proto_info.direct_sources
    srcs = [_generate_output_file(ctx, name, "_pb2.py") for name in proto_sources]
    transitive_sets = proto_info.transitive_descriptor_sets.to_list()
    ctx.actions.run(
        inputs = depset(
            direct = [proto_info.direct_descriptor_set],
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        outputs = srcs,
        executable = ctx.executable._protoc,
        arguments = [
                        "--python_out=" + _get_real_root(ctx, srcs[0]),
                        "--descriptor_set_in=" + ctx.configuration.host_path_separator.join([f.path for f in transitive_sets]),
                    ] +
                    [_get_real_short_path(file) for file in proto_sources],
        progress_message = "Generating Python protos for :" + ctx.label.name,
    )
    outs_depset = depset(srcs)
    return [
        PyInfo(transitive_sources = outs_depset),
    ]

_py_proto_library_aspect = aspect(
    attrs = {
        "_protoc": attr.label(
            executable = True,
            cfg = "exec",
            default = "//:protoc",
        ),
    },
    implementation = _py_proto_library_aspect_impl,
    provides = [
        PyInfo,
    ],
    attr_aspects = ["deps"],
)

py_proto_library = rule(
    output_to_genfiles = True,
    implementation = _py_proto_library_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_py_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)
