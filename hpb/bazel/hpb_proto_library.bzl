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
load("//bazel:upb_proto_library.bzl", "GeneratedSrcsInfo", "UpbWrappedCcInfo", "upb_proto_library_aspect")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/private:bazel_cc_proto_library.bzl", "cc_proto_aspect")
load("//bazel/private:upb_proto_library_internal/cc_library_func.bzl", "cc_library_func")  # buildifier: disable=bzl-visibility

def upb_use_cpp_toolchain():
    return use_cpp_toolchain()

def _filter_none(elems):
    return [e for e in elems if e]

_HpbWrappedCcInfo = provider("Provider for cc_info for hpb", fields = ["cc_info"])
_WrappedCcGeneratedSrcsInfo = provider("Provider for generated sources", fields = ["srcs"])

def _get_lang_toolchain(ctx):
    return ctx.attr._hpb_lang_toolchain[proto_common.ProtoLangToolchainInfo]

def _compile_hpb_protos(ctx, proto_info, proto_sources):
    if len(proto_sources) == 0:
        return GeneratedSrcsInfo(srcs = [], hdrs = [])

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

    proto_common.compile(
        actions = ctx.actions,
        proto_info = proto_info,
        proto_lang_toolchain_info = _get_lang_toolchain(ctx),
        generated_files = srcs + hdrs,
        experimental_exec_group = "proto_compiler",
    )

    return GeneratedSrcsInfo(srcs = srcs, hdrs = hdrs)

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
        DefaultInfo(files = depset(files + srcs.hdrs + srcs.srcs)),
        srcs,
        cc_info,
    ]

def _get_proto_deps(ctx):
    return [dep for dep in ctx.rule.attr.deps if ProtoInfo in dep]

def _upb_cc_proto_aspect_impl(target, ctx, cc_provider, file_provider):
    deps = _get_proto_deps(ctx) + ctx.attr._runtimedeps
    dep_ccinfos = [dep[CcInfo] for dep in deps if CcInfo in dep]
    dep_ccinfos += [dep[UpbWrappedCcInfo].cc_info for dep in deps if UpbWrappedCcInfo in dep]
    dep_ccinfos += [dep[_HpbWrappedCcInfo].cc_info for dep in deps if _HpbWrappedCcInfo in dep]

    backend = ctx.attr._hpb_backend[BuildSettingInfo].value

    if backend == "upb" and UpbWrappedCcInfo in target:
        dep_ccinfos.append(target[UpbWrappedCcInfo].cc_info)

    if backend == "cpp" and CcInfo in target:
        dep_ccinfos.append(target[CcInfo])

    proto_info = target[ProtoInfo]

    if not getattr(ctx.rule.attr, "srcs", []):
        # This target doesn't declare any sources, reexport all its deps instead.
        # This is known as an "alias library":
        # https://bazel.build/versions/6.4.0/reference/be/protocol-buffer#proto_library.srcs
        return [cc_provider(
            cc_info = cc_common.merge_cc_infos(direct_cc_infos = dep_ccinfos),
        ), file_provider(srcs = GeneratedSrcsInfo(srcs = [], hdrs = []))]
    else:
        files = _compile_hpb_protos(ctx, proto_info, proto_info.direct_sources)
        cc_info = cc_library_func(
            ctx = ctx,
            name = ctx.rule.attr.name + "_hpb",
            hdrs = files.hdrs,
            srcs = files.srcs,
            copts = [],
            dep_ccinfos = dep_ccinfos,
        )
        return [cc_provider(cc_info = cc_info), file_provider(srcs = files)]

def _upb_cc_proto_library_aspect_impl(target, ctx):
    return _upb_cc_proto_aspect_impl(target, ctx, _HpbWrappedCcInfo, _WrappedCcGeneratedSrcsInfo)

_hpb_proto_library_aspect_cpp = aspect(
    implementation = _upb_cc_proto_library_aspect_impl,
    attr_aspects = ["deps"],
    requires = [cc_proto_aspect],
    required_aspect_providers = [CcInfo],
    attrs = {
        "_runtimedeps": attr.label_list(
            default = [
                "//hpb/internal:os_macros",
                "@abseil-cpp//absl/log:absl_check",
                "@abseil-cpp//absl/strings",
                "@abseil-cpp//absl/status:statusor",
            ],
        ),
        "_hpb_backend": attr.label(
            default = "//hpb:hpb_backend",
        ),
        "_hpb_lang_toolchain": attr.label(
            default = "//hpb_generator:toolchain",
        ),
    },
    exec_groups = {
        "proto_compiler": exec_group(),
    },
    toolchains = upb_use_cpp_toolchain(),
    fragments = ["cpp"],
    required_providers = [ProtoInfo],
    provides = [
        _HpbWrappedCcInfo,
        _WrappedCcGeneratedSrcsInfo,
    ],
)

_hpb_proto_library_aspect_upb = aspect(
    implementation = _upb_cc_proto_library_aspect_impl,
    attr_aspects = ["deps"],
    requires = [upb_proto_library_aspect],
    required_aspect_providers = [UpbWrappedCcInfo],
    required_providers = [ProtoInfo],
    provides = [
        _HpbWrappedCcInfo,
        _WrappedCcGeneratedSrcsInfo,
    ],
    attrs = {
        "_runtimedeps": attr.label_list(
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
        "_hpb_backend": attr.label(
            default = "//hpb:hpb_backend",
        ),
        "_hpb_lang_toolchain": attr.label(
            default = "//hpb_generator:toolchain",
        ),
    },
    exec_groups = {
        "proto_compiler": exec_group(),
    },
    toolchains = upb_use_cpp_toolchain(),
    fragments = ["cpp"],
)

_hpb_proto_library_cpp = rule(
    implementation = _hpb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_hpb_proto_library_aspect_cpp],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)

_hpb_proto_library_upb = rule(
    implementation = _hpb_proto_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_hpb_proto_library_aspect_upb],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
)

def hpb_proto_library(name, deps, visibility = None, **kwargs):
    """
    Macro to create an hpb_proto_library target.

    This macro conditionally applies different aspects based on the
    //hpb:hpb_backend configuration.

    Args:
      name: The name of the target.
      deps: Propagated dependencies.
      visibility: Propagated visibility.
      **kwargs: Additional arguments to pass to the underlying rules.
    """
    cpp_target_name = name + "BACKEND_cpp"
    upb_target_name = name + "BACKEND_upb"

    # Instantiate the C++ specific version of the rule
    _hpb_proto_library_cpp(
        name = cpp_target_name,
        deps = deps,
        target_compatible_with = select({
            "//hpb:hpb_backend_cpp": [],
            "//hpb:hpb_backend_upb": [
                "@platforms//:incompatible",
            ],
        }),
        visibility = ["//visibility:private"],
        **kwargs
    )

    # Instantiate the UPB specific version of the rule
    _hpb_proto_library_upb(
        name = upb_target_name,
        deps = deps,
        target_compatible_with = select({
            "//hpb:hpb_backend_upb": [],
            "//hpb:hpb_backend_cpp": [
                "@platforms//:incompatible",
            ],
        }),
        visibility = ["//visibility:private"],
        **kwargs
    )

    # Alias to select the correct version based on the configuration
    native.alias(
        name = name,
        actual = select({
            "//hpb:hpb_backend_cpp": ":" + cpp_target_name,
            "//hpb:hpb_backend_upb": ":" + upb_target_name,
        }),
        visibility = visibility,
        **kwargs
    )
