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

load(":proto_common_wrapper.bzl", "output_dir", "proto_common_compile")
load("@rules_proto//proto:defs.bzl", "proto_common")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain", "use_cpp_toolchain")

# Generic support code #########################################################

# begin:github_only
_is_google3 = False
# end:github_only

# begin:google_only
# _is_google3 = True
# end:google_only

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

    blaze_only_args = {}

    if _is_google3:
        blaze_only_args["grep_includes"] = ctx.file._grep_includes

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
        **blaze_only_args
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
        **blaze_only_args
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

UpbWrappedCcInfo = provider("Provider for cc_info for protos", fields = ["cc_info", "cc_info_with_thunks"])
_UpbDefsWrappedCcInfo = provider("Provider for cc_info for protos", fields = ["cc_info"])
_UpbWrappedGeneratedSrcsInfo = provider("Provider for generated sources", fields = ["srcs"])
_WrappedDefsGeneratedSrcsInfo = provider(
    "Provider for generated reflective sources",
    fields = ["srcs"],
)

def _filter_none(elems):
    out = []
    for elem in elems:
        if elem:
            out.append(elem)
    return out

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

def _get_lang_toolchain(ctx, generator):
    lang_toolchain_name = "_" + generator + "_toolchain"
    return getattr(ctx.attr, lang_toolchain_name)[proto_common.ProtoLangToolchainInfo]

def _compile_upb_protos(ctx, generator, proto_info):
    proto_sources = proto_info.direct_sources
    if len(proto_sources) == 0:
        return [], []

    srcs = proto_common.declare_generated_files(
        ctx.actions,
        extension = "." + generator + ".c",
        proto_info = proto_info,
    )
    hdrs = proto_common.declare_generated_files(
        ctx.actions,
        extension = "." + generator + ".h",
        proto_info = proto_info,
    )

    proto_common_compile(
        ctx = ctx,
        proto_info = proto_info,
        proto_lang_toolchain_info = _get_lang_toolchain(ctx, generator),
        generated_files = srcs + hdrs,
    )

    if generator == "upb":
        thunks = proto_common.declare_generated_files(
            ctx.actions,
            extension = "." + generator + ".thunks.c",
            proto_info = proto_info,
        )
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
    else:
        thunks = []

    return srcs, hdrs, thunks

def _upb_proto_aspect_impl(target, ctx, generator, cc_provider, file_provider):
    proto_info = target[ProtoInfo]
    srcs, hdrs, thunks = _compile_upb_protos(ctx, generator, proto_info)
    files = GeneratedSrcsInfo(
        srcs = srcs,
        hdrs = hdrs,
        thunks = thunks,
    )
    runtime = _get_lang_toolchain(ctx, generator).runtime
    deps = ctx.rule.attr.deps + [runtime]
    dep_ccinfos = [dep[CcInfo] for dep in deps if CcInfo in dep]
    dep_ccinfos += [dep[UpbWrappedCcInfo].cc_info for dep in deps if UpbWrappedCcInfo in dep]
    dep_ccinfos += [dep[_UpbDefsWrappedCcInfo].cc_info for dep in deps if _UpbDefsWrappedCcInfo in dep]
    if generator == "upbdefs":
        if UpbWrappedCcInfo not in target:
            fail("Target should have UpbWrappedCcInfo provider")
        dep_ccinfos.append(target[UpbWrappedCcInfo].cc_info)
    cc_info = _cc_library_func(
        ctx = ctx,
        name = ctx.rule.attr.name + "." + generator,
        hdrs = files.hdrs,
        srcs = files.srcs,
        includes = [output_dir(ctx, proto_info)],
        copts = ctx.attr._copts[UpbProtoLibraryCoptsInfo].copts,
        dep_ccinfos = dep_ccinfos,
    )

    if files.thunks:
        cc_info_with_thunks = _cc_library_func(
            ctx = ctx,
            name = ctx.rule.attr.name + "." + generator + ".thunks",
            hdrs = [],
            srcs = files.thunks,
            includes = [output_dir(ctx, proto_info)],
            copts = ctx.attr._copts[UpbProtoLibraryCoptsInfo].copts,
            dep_ccinfos = dep_ccinfos + [cc_info],
        )
        wrapped_cc_info = cc_provider(
            cc_info = cc_info,
            cc_info_with_thunks = cc_info_with_thunks,
        )
    else:
        wrapped_cc_info = cc_provider(
            cc_info = cc_info,
        )
    return [
        wrapped_cc_info,
        file_provider(srcs = files),
    ]

def upb_proto_library_aspect_impl(target, ctx):
    return _upb_proto_aspect_impl(target, ctx, "upb", UpbWrappedCcInfo, _UpbWrappedGeneratedSrcsInfo)

def _upb_proto_reflection_library_aspect_impl(target, ctx):
    return _upb_proto_aspect_impl(target, ctx, "upbdefs", _UpbDefsWrappedCcInfo, _WrappedDefsGeneratedSrcsInfo)

def _maybe_add(d):
    if _is_google3:
        d["_grep_includes"] = attr.label(
            allow_single_file = True,
            cfg = "exec",
            default = "@bazel_tools//tools/cpp:grep-includes",
        )
    else:
        d["_cc_toolchain"] = attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        )
    return d

# upb_proto_library() ##########################################################

upb_proto_library_aspect = aspect(
    attrs = _maybe_add({
        "_copts": attr.label(
            default = "//:upb_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_upb_toolchain": attr.label(
            default = Label("//upbc:protoc-gen-upb_toolchain"),
            cfg = getattr(proto_common, "proto_lang_toolchain_cfg", "target"),
        ),
    }),
    implementation = upb_proto_library_aspect_impl,
    provides = [
        UpbWrappedCcInfo,
        _UpbWrappedGeneratedSrcsInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
    incompatible_use_toolchain_transition = True,
)

upb_proto_library = rule(
    implementation = _upb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [upb_proto_library_aspect],
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
        "_upbdefs_toolchain": attr.label(
            default = Label("//upbc:protoc-gen-upbdefs_toolchain"),
            cfg = getattr(proto_common, "proto_lang_toolchain_cfg", "target"),
        ),
    }),
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
)
