"""An indirection layer for referencing flags whether they are native or defined in Starlark."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

visibility([
    "//bazel/flags",
    "//bazel/private/...",
    "//third_party/grpc/bazel",
])

# Maps flag names to their native reference
_FLAGS = {
    "protocopt": struct(
        native = lambda ctx: getattr(ctx.fragments.proto, "experimental_protoc_opts"),
        default = [],
    ),
    "experimental_proto_descriptor_sets_include_source_info": struct(
        native = lambda ctx: getattr(ctx.attr, "_experimental_proto_descriptor_sets_include_source_info_native")[BuildSettingInfo].value,
        default = False,
    ),
    "proto_compiler": struct(native = lambda ctx: getattr(ctx.attr, "_proto_compiler_native")[BuildSettingInfo].value, default = "@bazel_tools//tools/proto:protoc"),
    "strict_proto_deps": struct(
        native = lambda ctx: getattr(ctx.attr, "_strict_proto_deps_native")[BuildSettingInfo].value,
        default = "error",
    ),
    "strict_public_imports": struct(
        native = lambda ctx: getattr(ctx.attr, "_strict_public_imports_native")[BuildSettingInfo].value,
        default = "off",
    ),
    "cc_proto_library_header_suffixes": struct(
        native = lambda ctx: getattr(ctx.fragments.proto, "cc_proto_library_header_suffixes"),
        default = [".pb.h"],
    ),
    "cc_proto_library_source_suffixes": struct(
        native = lambda ctx: getattr(ctx.fragments.proto, "cc_proto_library_source_suffixes"),
        default = [".pb.cc"],
    ),
    "proto_toolchain_for_java": struct(
        native = lambda ctx: getattr(ctx.attr, "_aspect_java_proto_toolchain"),
        default = "@bazel_tools//tools/proto:java_toolchain",
    ),
    "proto_toolchain_for_javalite": struct(
        native = lambda ctx: getattr(ctx.attr, "_aspect_proto_toolchain_for_javalite"),
        default = "@bazel_tools//tools/proto:javalite_toolchain",
    ),
    "proto_toolchain_for_cc": struct(
        native = lambda ctx: getattr(ctx.attr, "_aspect_cc_proto_toolchain"),
        default = "@bazel_tools//tools/proto:cc_toolchain",
    ),
}

def get_flag_value(ctx, flag_name):
    """Returns the value of the given flag in Starlark if it's set, otherwise reads the Java flag value, if the proto fragment exists.

    Args:
        ctx: The rule context.
        flag_name: The name of the flag to get the value for.

    Returns:
        The value of the flag.
    """

    # We probably got here from toolchains.find_toolchain. Leave the attribute alone.
    if flag_name not in _FLAGS:
        return getattr(ctx.attr, "_" + flag_name)

    starlark_flag = getattr(ctx.attr, "_" + flag_name)

    # Label flags don't have a BuildSettingInfo, just get the value.
    if "toolchain" in flag_name:
        starlark_flag_is_set = starlark_flag.label != _FLAGS[flag_name].default
        starlark_value = starlark_flag
    else:
        starlark_flag_is_set = starlark_flag[BuildSettingInfo].value != _FLAGS[flag_name].default
        starlark_value = starlark_flag[BuildSettingInfo].value

    # Starlark flags take precedence over native flags.
    # Also of course, use the Starlark value if the proto fragment no longer exists.
    if starlark_flag_is_set or not hasattr(ctx.fragments, "proto"):
        return starlark_value
    else:
        return _FLAGS[flag_name].native(ctx)
