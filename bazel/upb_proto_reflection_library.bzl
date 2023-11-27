"""upb_c_proto_reflection_library() exposes upb reflection for protobuf (foo.upbdefs.h)"""

load("//bazel:upb_minitable_proto_library.bzl", "UpbMinitableCcInfo", "upb_minitable_proto_library_aspect")
load("//bazel:upb_proto_library_internal/aspect.bzl", "upb_proto_aspect_impl")
load("//bazel:upb_proto_library_internal/cc_library_func.bzl", "upb_use_cpp_toolchain")
load("//bazel:upb_proto_library_internal/rule.bzl", "upb_proto_rule_impl")

_UpbDefsWrappedCcInfo = provider("Provider for cc_info for protos", fields = ["cc_info"])

_WrappedDefsGeneratedSrcsInfo = provider(
    "Provider for generated reflective sources",
    fields = ["srcs"],
)

def _upb_proto_reflection_library_aspect_impl(target, ctx):
    return upb_proto_aspect_impl(
        target = target,
        ctx = ctx,
        generator = "upbdefs",
        cc_provider = _UpbDefsWrappedCcInfo,
        dep_cc_provider = UpbMinitableCcInfo,
        file_provider = _WrappedDefsGeneratedSrcsInfo,
        provide_cc_shared_library_hints = False,
    )

_upb_proto_reflection_library_aspect = aspect(
    attrs = {
        "_copts": attr.label(
            default = "//upb:upb_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_upbdefs_toolchain": attr.label(
            default = Label("//upb_generator:protoc-gen-upbdefs_toolchain"),
            cfg = getattr(proto_common, "proto_lang_toolchain_cfg", "target"),
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
    },
    implementation = _upb_proto_reflection_library_aspect_impl,
    requires = [upb_minitable_proto_library_aspect],
    required_aspect_providers = [UpbMinitableCcInfo],
    provides = [
        _UpbDefsWrappedCcInfo,
        _WrappedDefsGeneratedSrcsInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = upb_use_cpp_toolchain(),
    exec_groups = {
        "proto_compiler": exec_group(),
    },
)

def _upb_proto_reflection_library_rule_impl(ctx):
    return upb_proto_rule_impl(ctx, _UpbDefsWrappedCcInfo, _WrappedDefsGeneratedSrcsInfo)

upb_proto_reflection_library = rule(
    output_to_genfiles = True,
    implementation = _upb_proto_reflection_library_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [_upb_proto_reflection_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
    provides = [CcInfo],
)
