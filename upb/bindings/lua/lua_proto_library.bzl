load("@bazel_skylib//lib:paths.bzl", "paths")

# Generic support code #########################################################

_is_bazel = not hasattr(native, "genmpm")

def _get_real_short_path(file):
    # For some reason, files from other archives have short paths that look like:
    #   ../com_google_protobuf/google/protobuf/descriptor.proto
    short_path = file.short_path
    if short_path.startswith("../"):
        second_slash = short_path.index("/", 3)
        short_path = short_path[second_slash + 1:]

    # Sometimes it has another few prefixes like:
    #   _virtual_imports/any_proto/google/protobuf/any.proto
    # We want just google/protobuf/any.proto.
    if short_path.startswith("_virtual_imports"):
        short_path = short_path.split("/", 2)[-1]
    return short_path

def _get_real_root(file):
    real_short_path = _get_real_short_path(file)
    return file.path[:-len(real_short_path) - 1]

def _generate_output_file(ctx, src, extension):
    real_short_path = _get_real_short_path(src)
    real_short_path = paths.relativize(real_short_path, ctx.label.package)
    output_filename = paths.replace_extension(real_short_path, extension)
    ret = ctx.actions.declare_file(output_filename)
    return ret

# upb_proto_library / upb_proto_reflection_library shared code #################

_LuaFiles = provider(fields = ["files"])

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
                        "--lua_out=" + _get_real_root(files[0]),
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
    if _LuaFiles not in dep:
        fail("proto_library rule must generate _LuaFiles (aspect should have handled this).")
    files = dep[_LuaFiles].files
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
    transitive = [dep[_LuaFiles].files for dep in deps if _LuaFiles in dep]
    return [_LuaFiles(files = depset(direct = files, transitive = transitive))]

# lua_proto_library() ##########################################################

_lua_proto_library_aspect = aspect(
    attrs = {
        "_upbc": attr.label(
            executable = True,
            cfg = "host",
            default = "//upb/bindings/lua:protoc-gen-lua",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "host",
            default = "@com_google_protobuf//:protoc",
        ),
    },
    implementation = _lua_proto_library_aspect_impl,
    provides = [_LuaFiles],
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
