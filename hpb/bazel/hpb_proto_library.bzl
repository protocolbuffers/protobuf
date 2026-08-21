# Copyright (c) 2024 Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Public rules for using hpb:
  - hpb_proto_library()
"""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "use_cpp_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")
load("//upb/bazel:upb_proto_library.bzl", "UpbWrappedCcInfo", "upb_proto_library_aspect")
load("//upb/bazel/private:upb_proto_library_internal/cc_library_func.bzl", "cc_library_func")  # buildifier: disable=bzl-visibility

def upb_use_cpp_toolchain():
    return use_cpp_toolchain()

def _filter_none(elems):
    return [e for e in elems if e]

_HpbWrappedCcInfo = provider("Provider for cc_info for hpb", fields = ["cc_info"])
_WrappedCcGeneratedSrcsInfo = provider("Provider for generated sources", fields = ["srcs"])

HpbGeneratedSrcsInfo = provider(
    "Provides generated headers, sources and metadata for hpb",
    fields = ["srcs", "hdrs", "metadata"],
)

def _get_lang_toolchain(ctx):
    return ctx.attr._hpb_lang_toolchain[proto_common.ProtoLangToolchainInfo]

def _add_annotate_headers_to_flag(out_flag):
    eq_idx = out_flag.find("=")
    if eq_idx == -1:
        return out_flag

    param_name = out_flag[:eq_idx]
    value = out_flag[eq_idx + 1:]

    colon_idx = value.rfind(":")
    if colon_idx == -1:
        args = ""
        output = value
    else:
        args = value[:colon_idx]
        output = value[colon_idx + 1:]

    if args:
        new_args = args + ",annotate_headers"
    else:
        new_args = "annotate_headers"

    return param_name + "=" + new_args + ":" + output

def _compile_hpb_protos(ctx, proto_info, proto_sources, generate_meta_data):
    if len(proto_sources) == 0:
        return HpbGeneratedSrcsInfo(srcs = [], hdrs = [], metadata = [])

    srcs = []
    srcs += proto_common.declare_generated_files(
        ctx.actions,
        extension = ".hpb.cc",
        proto_info = proto_info,
    )

    hdrs = []
    hdrs += proto_common.declare_generated_files(
        ctx.actions,
        extension = ".hpb.h",
        proto_info = proto_info,
    )

    metadata = []
    if generate_meta_data:
        metadata = proto_common.declare_generated_files(
            ctx.actions,
            extension = ".hpb.h.meta",
            proto_info = proto_info,
        )

    toolchain = _get_lang_toolchain(ctx)
    if generate_meta_data:
        out_flag = _add_annotate_headers_to_flag(toolchain.out_replacement_format_flag)

        toolchain = ProtoLangToolchainInfo(
            out_replacement_format_flag = out_flag,
            output_files = getattr(toolchain, "output_files", "legacy"),
            plugin_format_flag = toolchain.plugin_format_flag,
            plugin = toolchain.plugin,
            runtime = toolchain.runtime,
            provided_proto_sources = toolchain.provided_proto_sources,
            proto_compiler = toolchain.proto_compiler,
            protoc_opts = toolchain.protoc_opts,
            progress_message = toolchain.progress_message,
            mnemonic = toolchain.mnemonic,
            allowlist_different_package = getattr(toolchain, "allowlist_different_package", None),
            toolchain_type = getattr(toolchain, "toolchain_type", None),
        )

    proto_common.compile(
        actions = ctx.actions,
        proto_info = proto_info,
        proto_lang_toolchain_info = toolchain,
        generated_files = srcs + hdrs + metadata,
        experimental_exec_group = "proto_compiler",
    )

    return HpbGeneratedSrcsInfo(srcs = srcs, hdrs = hdrs, metadata = metadata)

def _hpb_proto_rule_impl(ctx):
    if len(ctx.attr.deps) != 1:
        fail("only one deps dependency allowed.")
    dep = ctx.attr.deps[0]

    if _WrappedCcGeneratedSrcsInfo in dep:
        srcs = dep[_WrappedCcGeneratedSrcsInfo].srcs
    else:
        fail("proto_library rule must generate _WrappedCcGeneratedSrcsInfo (aspect should have " +
             "handled this).")

    if _HpbWrappedCcInfo in dep:
        cc_info = dep[_HpbWrappedCcInfo].cc_info
    else:
        fail("proto_library rule must generate _HpbWrappedCcInfo (aspect should have handled this).")

    lib = cc_info.linking_context.linker_inputs.to_list()[0].libraries[0]
    files = _filter_none([
        lib.static_library,
        lib.pic_static_library,
        lib.dynamic_library,
    ])
    return [
        DefaultInfo(files = depset(files + srcs.hdrs + srcs.srcs + srcs.metadata)),
        srcs,
        cc_info,
    ]

def _get_proto_deps(ctx):
    return [dep for dep in ctx.rule.attr.deps if ProtoInfo in dep]

def _upb_cc_proto_aspect_impl(target, ctx, cc_provider, file_provider):
    generate_meta_data = False
    deps = _get_proto_deps(ctx) + ctx.attr._upbprotos
    dep_ccinfos = [dep[CcInfo] for dep in deps if CcInfo in dep]
    dep_ccinfos += [dep[UpbWrappedCcInfo].cc_info for dep in deps if UpbWrappedCcInfo in dep]
    dep_ccinfos += [dep[_HpbWrappedCcInfo].cc_info for dep in deps if _HpbWrappedCcInfo in dep]
    if UpbWrappedCcInfo not in target:
        fail("Target should have UpbWrappedCcInfo provider")
    dep_ccinfos.append(target[UpbWrappedCcInfo].cc_info)
    proto_info = target[ProtoInfo]

    if not getattr(ctx.rule.attr, "srcs", []):
        # This target doesn't declare any sources, reexport all its deps instead.
        # This is known as an "alias library":
        # https://bazel.build/versions/6.4.0/reference/be/protocol-buffer#proto_library.srcs
        return [cc_provider(
            cc_info = cc_common.merge_cc_infos(direct_cc_infos = dep_ccinfos),
        ), file_provider(srcs = HpbGeneratedSrcsInfo(srcs = [], hdrs = [], metadata = []))]
    else:
        files = _compile_hpb_protos(ctx, proto_info, proto_info.direct_sources, generate_meta_data)
        cc_info = cc_library_func(
            ctx = ctx,
            name = ctx.rule.attr.name + "_hpb",
            hdrs = files.hdrs,
            srcs = files.srcs,
            copts = [],
            dep_ccinfos = dep_ccinfos,
            non_compilation_additional_inputs = files.metadata,
        )
        return [cc_provider(cc_info = cc_info), file_provider(srcs = files)]

def _upb_cc_proto_library_aspect_impl(target, ctx):
    return _upb_cc_proto_aspect_impl(target, ctx, _HpbWrappedCcInfo, _WrappedCcGeneratedSrcsInfo)

_hpb_proto_library_aspect = aspect(
    attrs = {
        "_hpb_lang_toolchain": attr.label(
            default = "//hpb_generator:toolchain",
        ),
        "_upbprotos": attr.label_list(
            default = [
                # TODO: Add dependencies for cc runtime (absl/string etc..)
                "//upb:generated_cpp_support",
                "//hpb:generated_hpb_support",
                "//hpb/internal:os_macros",
                "@abseil-cpp//absl/log:absl_check",
                "@abseil-cpp//absl/strings",
                "@abseil-cpp//absl/status:statusor",
                "//hpb:repeated_field",
            ],
        ),
    },
    exec_groups = {
        "proto_compiler": exec_group(),
    },
    implementation = _upb_cc_proto_library_aspect_impl,
    provides = [
        _HpbWrappedCcInfo,
        _WrappedCcGeneratedSrcsInfo,
    ],
    required_aspect_providers = [
        UpbWrappedCcInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = upb_use_cpp_toolchain(),
    required_providers = [ProtoInfo],
)

hpb_proto_library = rule(
    implementation = _hpb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [
                upb_proto_library_aspect,
                _hpb_proto_library_aspect,
            ],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)
