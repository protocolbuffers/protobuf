"""An indirection layer for referencing flags whether they are native or defined in Starlark."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

visibility([
    "//bazel/flags",
    "//bazel/private/...",
    "//third_party/grpc/bazel",
])

def get_flag_value(ctx, flag_name):
    """Returns the value of the given flag attribute from rule context.

    Args:
        ctx: The rule context.
        flag_name: The name of the flag to get the value for.

    Returns:
        The value of the flag. If the value is a list, returns a mutable copy.
    """
    attr_val = getattr(ctx.attr, "_" + flag_name)
    if BuildSettingInfo in attr_val:
        val = attr_val[BuildSettingInfo].value
        if type(val) == "list":
            return list(val)
        return val
    if type(attr_val) == "list":
        return list(attr_val)
    return attr_val
