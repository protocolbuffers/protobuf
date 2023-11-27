"""upb_minitable_proto_library() exposes upb's generated minitables (foo.upb_minitable.h)"""

load("//bazel:upb_proto_library_internal/aspect.bzl", "upb_proto_aspect_impl")
load("//bazel:upb_proto_library_internal/cc_library_func.bzl", "upb_use_cpp_toolchain")
load("//bazel:upb_proto_library_internal/rule.bzl", "upb_proto_rule_impl")

UpbMinitableCcInfo = provider(
    "Provider for cc_info for protos",
    fields = ["cc_info"],
)

_UpbWrappedGeneratedSrcsInfo = provider(
    "Provider for generated sources",
    fields = ["srcs"],
)

def _upb_minitable_proto_library_aspect_impl(target, ctx):
    return upb_proto_aspect_impl(
        target = target,
        ctx = ctx,
        generator = "upb_minitable",
        cc_provider = UpbMinitableCcInfo,
        dep_cc_provider = None,
        file_provider = _UpbWrappedGeneratedSrcsInfo,
    )

def _get_upb_minitable_proto_library_aspect_provides():
    provides = [
        UpbMinitableCcInfo,
        _UpbWrappedGeneratedSrcsInfo,
    ]

    if hasattr(cc_common, "CcSharedLibraryHintInfo"):
        provides.append(cc_common.CcSharedLibraryHintInfo)
    elif hasattr(cc_common, "CcSharedLibraryHintInfo_6_X_getter_do_not_use"):
        # This branch can be deleted once 6.X is not supported by upb rules
        provides.append(cc_common.CcSharedLibraryHintInfo_6_X_getter_do_not_use)

    return provides

upb_minitable_proto_library_aspect = aspect(
    attrs = {
        "_copts": attr.label(
            default = "//upb:upb_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_upb_minitable_toolchain": attr.label(
            default = Label("//upb_generator:protoc-gen-upb_minitable_toolchain"),
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
        "_fasttable_enabled": attr.label(default = "//upb:fasttable_enabled"),
    },
    implementation = _upb_minitable_proto_library_aspect_impl,
    provides = _get_upb_minitable_proto_library_aspect_provides(),
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = upb_use_cpp_toolchain(),
    exec_groups = {
        "proto_compiler": exec_group(),
    },
)

def _upb_minitable_proto_library_rule_impl(ctx):
    return upb_proto_rule_impl(ctx, UpbMinitableCcInfo, _UpbWrappedGeneratedSrcsInfo)

upb_minitable_proto_library = rule(
    output_to_genfiles = True,
    implementation = _upb_minitable_proto_library_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [upb_minitable_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
    provides = [CcInfo],
)
