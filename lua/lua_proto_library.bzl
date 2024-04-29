# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""lua_proto_library(): a rule for building Lua protos."""

load("@bazel_skylib//lib:paths.bzl", "paths")

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
    if not _is_google3 and ctx.rule.attr.strip_import_prefix and ctx.rule.attr.strip_import_prefix != "/":
        package = package[len(ctx.rule.attr.strip_import_prefix):]
    real_short_path = _get_real_short_path(src)
    real_short_path = paths.relativize(real_short_path, package)
    output_filename = paths.replace_extension(real_short_path, extension)
    ret = ctx.actions.declare_file(output_filename)
    return ret

# upb_proto_library / upb_proto_reflection_library shared code #################

_LuaFilesInfo = provider(
    "A set of lua files generated from .proto files",
    fields = ["files"],
)

def _compile_upb_protos(ctx, proto_info, proto_sources):
    files = [_generate_output_file(ctx, name, "_pb.lua") for name in proto_sources]
    transitive_sets = proto_info.transitive_descriptor_sets.to_list()
    ctx.actions.run(
        inputs = depset(
            direct = [proto_info.direct_descriptor_set],
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        tools = [ctx.executable._upbc],
        outputs = files,
        executable = ctx.executable._protoc,
        arguments = [
                        "--lua_out=" + _get_real_root(ctx, files[0]),
                        "--plugin=protoc-gen-lua=" + ctx.executable._upbc.path,
                        "--descriptor_set_in=" + ctx.configuration.host_path_separator.join([f.path for f in transitive_sets]),
                    ] +
                    [_get_real_short_path(file) for file in proto_sources],
        progress_message = "Generating Lua protos for :" + ctx.label.name,
    )
    return files

def _lua_proto_rule_impl(ctx):
    if len(ctx.attr.deps) != 1:
        fail("only one deps dependency allowed.")
    dep = ctx.attr.deps[0]
    if _LuaFilesInfo not in dep:
        fail("proto_library rule must generate _LuaFilesInfo (aspect should have handled this).")
    files = dep[_LuaFilesInfo].files
    return [
        DefaultInfo(
            files = files,
            data_runfiles = ctx.runfiles(files = files.to_list()),
        ),
    ]

def _lua_proto_library_aspect_impl(target, ctx):
    proto_info = target[ProtoInfo]
    files = _compile_upb_protos(ctx, proto_info, proto_info.direct_sources)
    deps = ctx.rule.attr.deps
    transitive = [dep[_LuaFilesInfo].files for dep in deps if _LuaFilesInfo in dep]
    return [_LuaFilesInfo(files = depset(direct = files, transitive = transitive))]

# lua_proto_library() ##########################################################

_lua_proto_library_aspect = aspect(
    attrs = {
        "_upbc": attr.label(
            executable = True,
            cfg = "exec",
            default = "//lua:protoc-gen-lua",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "exec",
            default = "//:protoc",
        ),
    },
    implementation = _lua_proto_library_aspect_impl,
    provides = [_LuaFilesInfo],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
)

lua_proto_library = rule(
    output_to_genfiles = True,
    implementation = _lua_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_lua_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)
