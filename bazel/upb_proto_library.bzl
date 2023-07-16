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

"""Public rules for using upb protos:
  - upb_proto_library()
  - upb_proto_reflection_library()
"""

load("@bazel_skylib//lib:paths.bzl", "paths")

# begin:google_only
# load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain", "use_cpp_toolchain")
# end:google_only

# begin:github_only
# Compatibility code for Bazel 4.x. Remove this when we drop support for Bazel 4.x.
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")

def use_cpp_toolchain():
    return ["@bazel_tools//tools/cpp:toolchain_type"]
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

def _generate_include_path(src, out, extension):
    short_path = _get_real_short_path(src)
    short_path = paths.replace_extension(short_path, extension)
    if not out.path.endswith(short_path):
        fail("%s does not end with %s" % (out.path, short_path))

    return out.path[:-len(short_path)]

def _filter_none(elems):
    out = []
    for elem in elems:
        if elem:
            out.append(elem)
    return out

def _cc_library_func(ctx, name, hdrs, srcs, copts, includes, dep_ccinfos):
    """Like cc_library(), but callable from rules.

    Args:
      ctx: Rule context.
      name: Unique name used to generate output files.
      hdrs: Public headers that can be #included from other rules.
      srcs: C/C++ source files.
      copts: Additional options for cc compilation.
      includes: Additional include paths.
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

    (compilation_context, compilation_outputs) = cc_common.compile(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        name = name,
        srcs = srcs,
        includes = includes,
        public_hdrs = hdrs,
        user_compile_flags = copts,
        compilation_contexts = compilation_contexts,
    )

    # buildifier: disable=unused-variable
    (linking_context, linking_outputs) = cc_common.create_linking_context_from_compilation_outputs(
        actions = ctx.actions,
        name = name,
        feature_configuration = feature_configuration,
        cc_toolchain = toolchain,
        compilation_outputs = compilation_outputs,
        linking_contexts = linking_contexts,
        disallow_dynamic_library = cc_common.is_enabled(feature_configuration = feature_configuration, feature_name = "targets_windows"),
    )

    return CcInfo(
        compilation_context = compilation_context,
        linking_context = linking_context,
    )

# Dummy rule to expose select() copts to aspects  ##############################

UpbProtoLibraryCoptsInfo = provider(
    "Provides copts for upb proto targets",
    fields = {
        "copts": "copts for upb_proto_library()",
    },
)

def upb_proto_library_copts_impl(ctx):
    return UpbProtoLibraryCoptsInfo(copts = ctx.attr.copts)

upb_proto_library_copts = rule(
    implementation = upb_proto_library_copts_impl,
    attrs = {"copts": attr.string_list(default = [])},
)

# upb_proto_library / upb_proto_reflection_library shared code #################

GeneratedSrcsInfo = provider(
    "Provides generated headers and sources",
    fields = {
        "srcs": "list of srcs",
        "hdrs": "list of hdrs",
        "thunks": "Experimental, do not use. List of srcs defining C API. Incompatible with hdrs.",
        "includes": "list of extra includes",
    },
)

def _concat_lists(lists):
    ret = []
    for lst in lists:
        ret = ret + lst
    return ret

def _merge_generated_srcs(srcs):
    return GeneratedSrcsInfo(
        srcs = _concat_lists([s.srcs for s in srcs]),
        hdrs = _concat_lists([s.hdrs for s in srcs]),
        thunks = _concat_lists([s.thunks for s in srcs]),
        includes = _concat_lists([s.includes for s in srcs]),
    )

UpbWrappedCcInfo = provider("Provider for cc_info for protos", fields = ["cc_info", "cc_info_with_thunks"])

def _merge_wrapped_cc_infos(infos, cc_infos):
    return UpbWrappedCcInfo(
        cc_info = cc_common.merge_cc_infos(
            direct_cc_infos = cc_infos + [info.cc_info for info in infos],
        ),
        cc_info_with_thunks = cc_common.merge_cc_infos(
            direct_cc_infos = [info.cc_info_with_thunks for info in infos],
        ),
    )

_UpbDefsWrappedCcInfo = provider("Provider for cc_info for protos", fields = ["cc_info"])
_UpbWrappedGeneratedSrcsInfo = provider("Provider for generated sources", fields = ["srcs"])
_WrappedDefsGeneratedSrcsInfo = provider(
    "Provider for generated reflective sources",
    fields = ["srcs"],
)

def _generate_upb_protos(ctx, generator, proto_info, proto_sources):
    if len(proto_sources) == 0:
        return GeneratedSrcsInfo(srcs = [], hdrs = [], thunks = [], includes = [])

    ext = "." + generator
    tool = getattr(ctx.executable, "_gen_" + generator)
    srcs = [_generate_output_file(ctx, name, ext + ".c") for name in proto_sources]
    hdrs = [_generate_output_file(ctx, name, ext + ".h") for name in proto_sources]
    thunks = []
    if generator == "upb":
        thunks = [_generate_output_file(ctx, name, ext + ".thunks.c") for name in proto_sources]
    transitive_sets = proto_info.transitive_descriptor_sets.to_list()

    args = ctx.actions.args()
    args.use_param_file(param_file_arg = "@%s")
    args.set_param_file_format("multiline")

    args.add("--" + generator + "_out=" + _get_real_root(ctx, srcs[0]))
    args.add("--plugin=protoc-gen-" + generator + "=" + tool.path)
    args.add("--descriptor_set_in=" + ctx.configuration.host_path_separator.join([f.path for f in transitive_sets]))
    args.add_all(proto_sources, map_each = _get_real_short_path)

    ctx.actions.run(
        inputs = depset(
            direct = [proto_info.direct_descriptor_set],
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        tools = [tool],
        outputs = srcs + hdrs,
        executable = ctx.executable._protoc,
        arguments = [args],
        progress_message = "Generating upb protos for :" + ctx.label.name,
        mnemonic = "GenUpbProtos",
    )
    if generator == "upb":
        ctx.actions.run_shell(
            inputs = hdrs,
            outputs = thunks,
            command = " && ".join([
                "sed 's/UPB_INLINE //' {} > {}".format(hdr.path, thunk.path)
                for (hdr, thunk) in zip(hdrs, thunks)
            ]),
            progress_message = "Generating thunks for upb protos API for: " + ctx.label.name,
            mnemonic = "GenUpbProtosThunks",
        )
    return GeneratedSrcsInfo(
        srcs = srcs,
        hdrs = hdrs,
        thunks = thunks,
        includes = [_generate_include_path(proto_sources[0], hdrs[0], ext + ".h")],
    )

def _upb_proto_rule_impl(ctx):
    if len(ctx.attr.deps) != 1:
        fail("only one deps dependency allowed.")
    dep = ctx.attr.deps[0]

    if _WrappedDefsGeneratedSrcsInfo in dep:
        srcs = dep[_WrappedDefsGeneratedSrcsInfo].srcs
    elif _UpbWrappedGeneratedSrcsInfo in dep:
        srcs = dep[_UpbWrappedGeneratedSrcsInfo].srcs
    else:
        fail("proto_library rule must generate _UpbWrappedGeneratedSrcsInfo or " +
             "_WrappedDefsGeneratedSrcsInfo (aspect should have handled this).")

    if _UpbDefsWrappedCcInfo in dep:
        cc_info = dep[_UpbDefsWrappedCcInfo].cc_info
    elif UpbWrappedCcInfo in dep:
        cc_info = dep[UpbWrappedCcInfo].cc_info
    else:
        fail("proto_library rule must generate UpbWrappedCcInfo or " +
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

def _generate_name(ctx, generator, thunks = False):
    if thunks:
        return ctx.rule.attr.name + "." + generator + ".thunks"
    return ctx.rule.attr.name + "." + generator

def _get_dep_cc_info(target, ctx, generator):
    deps = ctx.rule.attr.deps + getattr(ctx.attr, "_" + generator)
    dep_ccinfos = [dep[CcInfo] for dep in deps if CcInfo in dep]
    dep_ccinfos += [dep[_UpbDefsWrappedCcInfo].cc_info for dep in deps if _UpbDefsWrappedCcInfo in dep]

    dep_wrapped_infos = [dep[UpbWrappedCcInfo] for dep in deps if UpbWrappedCcInfo in dep]
    if generator == "upbdefs":
        if UpbWrappedCcInfo not in target:
            fail("Target should have UpbWrappedCcInfo provider")
        dep_wrapped_infos.append(target[UpbWrappedCcInfo])

    return _merge_wrapped_cc_infos(dep_wrapped_infos, dep_ccinfos)

def _compile_upb_protos(ctx, files, generator, dep_wrapped_ccinfo, cc_provider):
    cc_info = _cc_library_func(
        ctx = ctx,
        name = _generate_name(ctx, generator),
        hdrs = files.hdrs,
        srcs = files.srcs,
        includes = files.includes,
        copts = ctx.attr._copts[UpbProtoLibraryCoptsInfo].copts,
        dep_ccinfos = [dep_wrapped_ccinfo.cc_info],
    )

    if files.thunks:
        cc_info_with_thunks = _cc_library_func(
            ctx = ctx,
            name = _generate_name(ctx, generator, files.thunks),
            hdrs = [],
            srcs = files.thunks,
            includes = files.includes,
            copts = ctx.attr._copts[UpbProtoLibraryCoptsInfo].copts,
            dep_ccinfos = [dep_wrapped_ccinfo.cc_info, cc_info],
        )
        return cc_provider(
            cc_info = cc_info,
            cc_info_with_thunks = cc_info_with_thunks,
        )
    else:
        return cc_provider(
            cc_info = cc_info,
        )

def _get_hint_providers(ctx, generator):
    if generator not in _GENERATORS:
        fail("Please add new generator '{}' to _GENERATORS list".format(generator))

    possible_owners = []
    for generator in _GENERATORS:
        possible_owners.append(ctx.label.relative(_generate_name(ctx, generator)))
        possible_owners.append(ctx.label.relative(_generate_name(ctx, generator, thunks = True)))

    if hasattr(cc_common, "CcSharedLibraryHintInfo"):
        return [cc_common.CcSharedLibraryHintInfo(owners = possible_owners)]
    elif hasattr(cc_common, "CcSharedLibraryHintInfo_6_X_constructor_do_not_use"):
        # This branch can be deleted once 6.X is not supported by upb rules
        return [cc_common.CcSharedLibraryHintInfo_6_X_constructor_do_not_use(owners = possible_owners)]

    return []

def _upb_proto_aspect_impl(target, ctx, generator, cc_provider, file_provider, provide_cc_shared_library_hints = True):
    dep_wrapped_ccinfo = _get_dep_cc_info(target, ctx, generator)
    if not getattr(ctx.rule.attr, "srcs", []):
        # This target doesn't declare any sources, reexport all its deps instead.
        # This is known as an "alias library":
        #    https://bazel.build/reference/be/protocol-buffer#proto_library.srcs
        files = _merge_generated_srcs([dep[file_provider].srcs for dep in ctx.rule.attr.deps])
        wrapped_cc_info = dep_wrapped_ccinfo
    else:
        proto_info = target[ProtoInfo]
        files = _generate_upb_protos(
            ctx,
            generator,
            proto_info,
            proto_info.direct_sources,
        )
        wrapped_cc_info = _compile_upb_protos(
            ctx,
            files,
            generator,
            dep_wrapped_ccinfo,
            cc_provider,
        )

    hints = _get_hint_providers(ctx, generator) if provide_cc_shared_library_hints else []

    return hints + [
        file_provider(srcs = files),
        wrapped_cc_info,
    ]

_GENERATORS = ["upb", "upbdefs"]

def upb_proto_library_aspect_impl(target, ctx):
    return _upb_proto_aspect_impl(target, ctx, "upb", UpbWrappedCcInfo, _UpbWrappedGeneratedSrcsInfo)

def _upb_proto_reflection_library_aspect_impl(target, ctx):
    return _upb_proto_aspect_impl(target, ctx, "upbdefs", _UpbDefsWrappedCcInfo, _WrappedDefsGeneratedSrcsInfo, provide_cc_shared_library_hints = False)

# upb_proto_library() ##########################################################

def _get_upb_proto_library_aspect_provides():
    provides = [
        UpbWrappedCcInfo,
        _UpbWrappedGeneratedSrcsInfo,
    ]

    if hasattr(cc_common, "CcSharedLibraryHintInfo"):
        provides.append(cc_common.CcSharedLibraryHintInfo)
    elif hasattr(cc_common, "CcSharedLibraryHintInfo_6_X_getter_do_not_use"):
        # This branch can be deleted once 6.X is not supported by upb rules
        provides.append(cc_common.CcSharedLibraryHintInfo_6_X_getter_do_not_use)

    return provides

upb_proto_library_aspect = aspect(
    attrs = {
        "_copts": attr.label(
            default = "//:upb_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_gen_upb": attr.label(
            executable = True,
            cfg = "exec",
            default = "//upbc:protoc-gen-upb_stage1",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "exec",
            default = "@com_google_protobuf//:protoc",
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
        "_upb": attr.label_list(default = [
            "//:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ]),
        "_fasttable_enabled": attr.label(default = "//:fasttable_enabled"),
    },
    implementation = upb_proto_library_aspect_impl,
    provides = _get_upb_proto_library_aspect_provides(),
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
    incompatible_use_toolchain_transition = True,
)

upb_proto_library = rule(
    output_to_genfiles = True,
    implementation = _upb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [upb_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
    provides = [CcInfo],
)

# upb_proto_reflection_library() ###############################################

_upb_proto_reflection_library_aspect = aspect(
    attrs = {
        "_copts": attr.label(
            default = "//:upb_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_gen_upbdefs": attr.label(
            executable = True,
            cfg = "exec",
            default = "//upbc:protoc-gen-upbdefs",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "exec",
            default = "@com_google_protobuf//:protoc",
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
        "_upbdefs": attr.label_list(
            default = [
                "//:generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
            ],
        ),
    },
    implementation = _upb_proto_reflection_library_aspect_impl,
    provides = [
        _UpbDefsWrappedCcInfo,
        _WrappedDefsGeneratedSrcsInfo,
    ],
    required_aspect_providers = [
        UpbWrappedCcInfo,
        _UpbWrappedGeneratedSrcsInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
    incompatible_use_toolchain_transition = True,
)

upb_proto_reflection_library = rule(
    output_to_genfiles = True,
    implementation = _upb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [
                upb_proto_library_aspect,
                _upb_proto_reflection_library_aspect,
            ],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
    provides = [CcInfo],
)
