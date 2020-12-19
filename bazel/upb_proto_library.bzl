"""Public rules for using upb protos:
  - upb_proto_library()
  - upb_proto_reflection_library()
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@rules_proto//proto:defs.bzl", "ProtoInfo")  # copybara:strip_for_google3

# Generic support code #########################################################

_is_bazel = True  # copybara:replace_for_google3 _is_bazel = False

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

def _get_real_root(file):
    real_short_path = _get_real_short_path(file)
    return file.path[:-len(real_short_path) - 1]

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

def _cc_library_func(ctx, name, hdrs, srcs, copts, dep_ccinfos):
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
        user_compile_flags = copts,
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

# Build setting for whether fasttable code generation is enabled ###############

_FastTableEnabled = provider(
    fields = {
        "enabled": "whether fasttable is enabled",
    },
)

def fasttable_enabled_impl(ctx):
    raw_setting = ctx.build_setting_value

    if raw_setting:
        # TODO(haberman): check that the target CPU supports fasttable.
        pass

    return _FastTableEnabled(enabled = raw_setting)

upb_fasttable_enabled = rule(
    implementation = fasttable_enabled_impl,
    build_setting = config.bool(flag = True),
)

# Dummy rule to expose select() copts to aspects  ##############################

_UpbProtoLibraryCopts = provider(
    fields = {
        "copts": "copts for upb_proto_library()",
    },
)

def upb_proto_library_copts_impl(ctx):
    return _UpbProtoLibraryCopts(copts = ctx.attr.copts)

upb_proto_library_copts = rule(
    implementation = upb_proto_library_copts_impl,
    attrs = {"copts": attr.string_list(default = [])},
)

# upb_proto_library / upb_proto_reflection_library shared code #################

GeneratedSrcsInfo = provider(
    fields = {
        "srcs": "list of srcs",
        "hdrs": "list of hdrs",
    },
)

_UpbWrappedCcInfo = provider(fields = ["cc_info"])
_UpbDefsWrappedCcInfo = provider(fields = ["cc_info"])
_WrappedGeneratedSrcsInfo = provider(fields = ["srcs"])
_WrappedDefsGeneratedSrcsInfo = provider(fields = ["srcs"])

def _compile_upb_protos(ctx, generator, proto_info, proto_sources):
    if len(proto_sources) == 0:
        return GeneratedSrcsInfo(srcs = [], hdrs = [])

    ext = "." + generator
    tool = getattr(ctx.executable, "_gen_" + generator)
    srcs = [_generate_output_file(ctx, name, ext + ".c") for name in proto_sources]
    hdrs = [_generate_output_file(ctx, name, ext + ".h") for name in proto_sources]
    transitive_sets = proto_info.transitive_descriptor_sets.to_list()
    fasttable_enabled = (hasattr(ctx.attr, "_fasttable_enabled") and
                         ctx.attr._fasttable_enabled[_FastTableEnabled].enabled)
    codegen_params = "fasttable:" if fasttable_enabled else ""
    ctx.actions.run(
        inputs = depset(
            direct = [proto_info.direct_descriptor_set],
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        tools = [tool],
        outputs = srcs + hdrs,
        executable = ctx.executable._protoc,
        arguments = [
                        "--" + generator + "_out=" + codegen_params + _get_real_root(srcs[0]),
                        "--plugin=protoc-gen-" + generator + "=" + tool.path,
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

    if _WrappedDefsGeneratedSrcsInfo in dep:
        srcs = dep[_WrappedDefsGeneratedSrcsInfo].srcs
    elif _WrappedGeneratedSrcsInfo in dep:
        srcs = dep[_WrappedGeneratedSrcsInfo].srcs
    else:
        fail("proto_library rule must generate _WrappedGeneratedSrcsInfo or " +
             "_WrappedDefsGeneratedSrcsInfo (aspect should have handled this).")

    if _UpbDefsWrappedCcInfo in dep:
        cc_info = dep[_UpbDefsWrappedCcInfo].cc_info
    elif _UpbWrappedCcInfo in dep:
        cc_info = dep[_UpbWrappedCcInfo].cc_info
    else:
        fail("proto_library rule must generate _UpbWrappedCcInfo or " +
             "_UpbDefsWrappedCcInfo (aspect should have handled this).")

    lib = cc_info.linking_context.linker_inputs.to_list()[0].libraries[0]
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

def _upb_proto_aspect_impl(target, ctx, generator, cc_provider, file_provider):
    proto_info = target[ProtoInfo]
    files = _compile_upb_protos(ctx, generator, proto_info, proto_info.direct_sources)
    deps = ctx.rule.attr.deps + getattr(ctx.attr, "_" + generator)
    dep_ccinfos = [dep[CcInfo] for dep in deps if CcInfo in dep]
    dep_ccinfos += [dep[_UpbWrappedCcInfo].cc_info for dep in deps if _UpbWrappedCcInfo in dep]
    dep_ccinfos += [dep[_UpbDefsWrappedCcInfo].cc_info for dep in deps if _UpbDefsWrappedCcInfo in dep]
    if generator == "upbdefs":
        if _UpbWrappedCcInfo not in target:
            fail("Target should have _UpbDefsWrappedCcInfo provider")
        dep_ccinfos += [target[_UpbWrappedCcInfo].cc_info]
    cc_info = _cc_library_func(
        ctx = ctx,
        name = ctx.rule.attr.name + "." + generator,
        hdrs = files.hdrs,
        srcs = files.srcs,
        copts = ctx.attr._copts[_UpbProtoLibraryCopts].copts,
        dep_ccinfos = dep_ccinfos,
    )
    return [cc_provider(cc_info = cc_info), file_provider(srcs = files)]

def _upb_proto_library_aspect_impl(target, ctx):
    return _upb_proto_aspect_impl(target, ctx, "upb", _UpbWrappedCcInfo, _WrappedGeneratedSrcsInfo)

def _upb_proto_reflection_library_aspect_impl(target, ctx):
    return _upb_proto_aspect_impl(target, ctx, "upbdefs", _UpbDefsWrappedCcInfo, _WrappedDefsGeneratedSrcsInfo)

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
        "_copts": attr.label(
            default = "//:upb_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_gen_upb": attr.label(
            executable = True,
            cfg = "host",
            default = "//upbc:protoc-gen-upb",
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
            "//:upb",
        ]),
        "_fasttable_enabled": attr.label(default = "//:fasttable_enabled"),
    }),
    implementation = _upb_proto_library_aspect_impl,
    provides = [
        _UpbWrappedCcInfo,
        _WrappedGeneratedSrcsInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    incompatible_use_toolchain_transition = True,
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
        "_copts": attr.label(
            default = "//:upb_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_gen_upbdefs": attr.label(
            executable = True,
            cfg = "host",
            default = "//upbc:protoc-gen-upbdefs",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "host",
            default = "@com_google_protobuf//:protoc",
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
        "_upbdefs": attr.label_list(
            default = [
                "//:upb",
                "//:reflection",
            ],
        ),
    }),
    implementation = _upb_proto_reflection_library_aspect_impl,
    provides = [
        _UpbDefsWrappedCcInfo,
        _WrappedDefsGeneratedSrcsInfo,
    ],
    required_aspect_providers = [
        _UpbWrappedCcInfo,
        _WrappedGeneratedSrcsInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = ["@bazel_tools//tools/cpp:toolchain_type"],
    incompatible_use_toolchain_transition = True,
)

upb_proto_reflection_library = rule(
    output_to_genfiles = True,
    implementation = _upb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [
                _upb_proto_library_aspect,
                _upb_proto_reflection_library_aspect,
            ],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)
