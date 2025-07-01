"""upb_c_proto_library() exposes upb's generated C API for protobuf (foo.upb.h)"""

load("//bazel:upb_minitable_proto_library.bzl", "UpbMinitableCcInfo", "upb_minitable_proto_library_aspect")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/private:upb_proto_library_internal/aspect.bzl", "upb_proto_aspect_impl")
load("//bazel/private:upb_proto_library_internal/cc_library_func.bzl", "upb_use_cpp_toolchain")
load("//bazel/private:upb_proto_library_internal/rule.bzl", "upb_proto_rule_impl")

UpbWrappedCcInfo = provider(
    "Provider for cc_info for protos",
    fields = ["cc_info"],
)

_UpbWrappedGeneratedSrcsInfo = provider(
    "Provider for generated sources",
    fields = ["srcs"],
)

def _upb_c_proto_library_aspect_impl(target, ctx):
    return upb_proto_aspect_impl(
        target = target,
        ctx = ctx,
        generator = "upb",
        cc_provider = UpbWrappedCcInfo,
        dep_cc_provider = UpbMinitableCcInfo,
        file_provider = _UpbWrappedGeneratedSrcsInfo,
        provide_cc_shared_library_hints = False,
    )

upb_c_proto_library_aspect = aspect(
    attrs = {
        "_copts": attr.label(
            default = "//upb:upb_proto_library_copts",
        ),
        "_upb_toolchain": attr.label(
            default = Label("//upb_generator/c:toolchain"),
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
    },
    implementation = _upb_c_proto_library_aspect_impl,
    requires = [upb_minitable_proto_library_aspect],
    required_aspect_providers = [UpbMinitableCcInfo],
    provides = [
        UpbWrappedCcInfo,
        _UpbWrappedGeneratedSrcsInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = upb_use_cpp_toolchain(),
    exec_groups = {
        "proto_compiler": exec_group(),
    },
    required_providers = [ProtoInfo],
)

def _upb_c_proto_library_rule_impl(ctx):
    return upb_proto_rule_impl(ctx, UpbWrappedCcInfo, _UpbWrappedGeneratedSrcsInfo)

upb_c_proto_library = rule(
    implementation = _upb_c_proto_library_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [upb_c_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
    provides = [CcInfo],
)
