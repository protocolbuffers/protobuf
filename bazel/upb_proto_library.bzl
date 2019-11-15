"""Public rules for using upb protos:
  - upb_proto_library()
  - upb_proto_reflection_library()
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")

# copybara:strip_for_google3_begin
load("@bazel_skylib//lib:versions.bzl", "versions")
load("@upb_bazel_version//:bazel_version.bzl", "bazel_version")
# copybara:strip_end

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

def _get_real_roots(files):
    roots = {}
    for file in files:
        real_root = _get_real_root(file)
        if real_root:
            roots[real_root] = True
    return roots.keys()

def _generate_output_file(ctx, src, extension):
    real_short_path = _get_real_short_path(src)
    real_short_path = paths.relativize(real_short_path, ctx.label.package)
    output_filename = paths.replace_extension(real_short_path, extension)
    ret = ctx.actions.declare_file(output_filename)
    return ret

def _filter_none(elems):
    out = []
    for elem in elems:
        if elem:
            out.append(elem)
    return out

def _cc_library_func(ctx, name, hdrs, srcs, dep_ccinfos):
    """Like cc_library(), but callable from rules.

    Args:
      ctx: Rule context.
      name: Unique name used to generate output files.
      hdrs: Public headers that can be #included from other rules.
      srcs: C/C++ source files.
      dep_ccinfos: CcInfo providers of dependencies we should build/link against.

    Returns:
      CcInfo provider for this compilation.
    """

    compilation_contexts = [info.compilation_context for info in dep_ccinfos]
    linking_contexts = [info.linking_context for info in dep_ccinfos]
    toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    # copybara:strip_for_google3_begin
    if bazel_version == "0.24.1":
        # Compatibility code until gRPC is on 0.25.2 or later.
        compilation_info = cc_common.compile(
            ctx = ctx,
            feature_configuration = feature_configuration,
            cc_toolchain = toolchain,
            srcs = srcs,
            hdrs = hdrs,
            compilation_contexts = compilation_contexts,
        )
        linking_info = cc_common.link(
            ctx = ctx,
            feature_configuration = feature_configuration,
            cc_toolchain = toolchain,
            cc_compilation_outputs = compilation_info.cc_compilation_outputs,
            linking_contexts = linking_contexts,
        )
        return CcInfo(
            compilation_context = compilation_info.compilation_context,
            linking_context = linking_info.linking_context,
        )

    if not versions.is_at_least("0.25.2", bazel_version):
        fail("upb requires Bazel >=0.25.2 or 0.24.1")

    # copybara:strip_end

    blaze_only_args = {}

    if not _is_bazel:
        blaze_only_args["grep_includes"] = ctx.file._grep_includes

    (compilation_context, compilation_outputs) = cc_common.compile(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        name = name,
        srcs = srcs,
        public_hdrs = hdrs,
        compilation_contexts = compilation_contexts,
        **blaze_only_args
    )
    (linking_context, linking_outputs) = cc_common.create_linking_context_from_compilation_outputs(
        actions = ctx.actions,
        name = name,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        compilation_outputs = compilation_outputs,
        linking_contexts = linking_contexts,
        **blaze_only_args
    )

    return CcInfo(
        compilation_context = compilation_context,
        linking_context = linking_context,
    )

# upb_proto_library / upb_proto_reflection_library shared code #################

GeneratedSrcsInfo = provider(
    fields = {
        "srcs": "list of srcs",
        "hdrs": "list of hdrs",
    },
)

_WrappedCcInfo = provider(fields = ["cc_info"])
_WrappedGeneratedSrcsInfo = provider(fields = ["srcs"])

def _compile_upb_protos(ctx, proto_info, proto_sources, ext):
    srcs = [_generate_output_file(ctx, name, ext + ".c") for name in proto_sources]
    hdrs = [_generate_output_file(ctx, name, ext + ".h") for name in proto_sources]
    transitive_sets = proto_info.transitive_descriptor_sets.to_list()
    ctx.actions.run(
        inputs = depset(
            direct = [proto_info.direct_descriptor_set],
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        tools = [ctx.executable._upbc],
        outputs = srcs + hdrs,
        executable = ctx.executable._protoc,
        arguments = [
                        "--upb_out=" + _get_real_root(srcs[0]),
                        "--plugin=protoc-gen-upb=" + ctx.executable._upbc.path,
                        "--descriptor_set_in=" + ctx.configuration.host_path_separator.join([f.path for f in transitive_sets]),
                    ] +
                    [_get_real_short_path(file) for file in proto_sources],
        progress_message = "Generating upb protos for :" + ctx.label.name,
    )
    return GeneratedSrcsInfo(srcs = srcs, hdrs = hdrs)

def _upb_proto_rule_impl(ctx):
    if len(ctx.attr.deps) != 1:
        fail("only one deps dependency allowed.")
    dep = ctx.attr.deps[0]
    if _WrappedCcInfo not in dep or _WrappedGeneratedSrcsInfo not in dep:
        fail("proto_library rule must generate _WrappedCcInfo and " +
             "_WrappedGeneratedSrcsInfo (aspect should have handled this).")
    cc_info = dep[_WrappedCcInfo].cc_info
    srcs = dep[_WrappedGeneratedSrcsInfo].srcs
    if type(cc_info.linking_context.libraries_to_link) == "list":
        lib = cc_info.linking_context.libraries_to_link[0]
    else:
        lib = cc_info.linking_context.libraries_to_link.to_list()[0]
    files = _filter_none([
        lib.static_library,
        lib.pic_static_library,
        lib.dynamic_library,
    ])
    return [
        DefaultInfo(files = depset(files + srcs.hdrs + srcs.srcs)),
        srcs,
        cc_info,
    ]

def _upb_proto_aspect_impl(target, ctx):
    proto_info = target[ProtoInfo]
    files = _compile_upb_protos(ctx, proto_info, proto_info.direct_sources, ctx.attr._ext)
    deps = ctx.rule.attr.deps + ctx.attr._upb
    dep_ccinfos = [dep[CcInfo] for dep in deps if CcInfo in dep]
    dep_ccinfos += [dep[_WrappedCcInfo].cc_info for dep in deps if _WrappedCcInfo in dep]
    cc_info = _cc_library_func(
        ctx = ctx,
        name = ctx.rule.attr.name + ctx.attr._ext,
        hdrs = files.hdrs,
        srcs = files.srcs,
        dep_ccinfos = dep_ccinfos,
    )
    return [_WrappedCcInfo(cc_info = cc_info), _WrappedGeneratedSrcsInfo(srcs = files)]

def _maybe_add(d):
    if not _is_bazel:
        d["_grep_includes"] = attr.label(
            allow_single_file = True,
            cfg = "host",
            default = "//tools/cpp:grep-includes",
        )
    return d

# upb_proto_library() ##########################################################

_upb_proto_library_aspect = aspect(
    attrs = _maybe_add({
        "_upbc": attr.label(
            executable = True,
            cfg = "host",
            default = "//:protoc-gen-upb",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "host",
            default = "@com_google_protobuf//:protoc",
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
        "_upb": attr.label_list(default = [
            "//:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
            "//:upb"
        ]),
        "_ext": attr.string(default = ".upb"),
    }),
    implementation = _upb_proto_aspect_impl,
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
)

upb_proto_library = rule(
    output_to_genfiles = True,
    implementation = _upb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_upb_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)

# upb_proto_reflection_library() ###############################################

_upb_proto_reflection_library_aspect = aspect(
    attrs = _maybe_add({
        "_upbc": attr.label(
            executable = True,
            cfg = "host",
            default = "//:protoc-gen-upb",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "host",
            default = "@com_google_protobuf//:protoc",
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
        "_upb": attr.label_list(
            default = [
                "//:upb",
                "//:reflection",
            ],
        ),
        "_ext": attr.string(default = ".upbdefs"),
    }),
    implementation = _upb_proto_aspect_impl,
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
)

upb_proto_reflection_library = rule(
    output_to_genfiles = True,
    implementation = _upb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_upb_proto_reflection_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)
