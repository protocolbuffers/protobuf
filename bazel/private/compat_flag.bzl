# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Helper rules and macros for custom proto flags."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _compat_bool_rule_impl(ctx):
    starlark_val = ctx.build_setting_value

    # 1. Starlark flag takes precedence if explicitly passed (not "default")
    if starlark_val != "default":
        starlark_lower = starlark_val.lower()
        true_lower = [v.lower() for v in ctx.attr.true_values]
        false_lower = [v.lower() for v in ctx.attr.false_values]
        if starlark_lower in true_lower:
            return [BuildSettingInfo(value = True)]
        if starlark_lower in false_lower:
            return [BuildSettingInfo(value = False)]
        allowed = ctx.attr.true_values + ctx.attr.false_values + ["default"]
        fail("Invalid value '%s' for flag %s. Allowed values are: %s" % (
            starlark_val,
            ctx.label,
            allowed,
        ))

    # 2. Native flag fallback (if present on ctx.fragments.proto)
    if hasattr(ctx.fragments, "proto") and ctx.attr.fragment_field:
        if hasattr(ctx.fragments.proto, ctx.attr.fragment_field):
            native_val = getattr(ctx.fragments.proto, ctx.attr.fragment_field)
            if type(native_val) == "bool":
                return [BuildSettingInfo(value = native_val)]
            if type(native_val) == "string":
                val_lower = native_val.lower()
                true_lower = [v.lower() for v in ctx.attr.true_values]
                false_lower = [v.lower() for v in ctx.attr.false_values]
                if val_lower in true_lower:
                    return [BuildSettingInfo(value = True)]
                if val_lower in false_lower:
                    return [BuildSettingInfo(value = False)]

    # 3. Default fallback
    return [BuildSettingInfo(value = ctx.attr.default_value)]

_compat_bool_rule = rule(
    implementation = _compat_bool_rule_impl,
    build_setting = config.string(flag = True),
    fragments = ["proto"],
    attrs = {
        "fragment_field": attr.string(),
        "true_values": attr.string_list(default = []),
        "false_values": attr.string_list(default = []),
        "default_value": attr.bool(default = False),
        "scope": attr.string(),
    },
)

def compat_bool_flag(
        *,
        name,
        fragment_field = None,
        build_setting_default = False,
        values = None,
        **kwargs):
    """Creates a custom build setting flag reconciling Starlark/fragments.

    Args:
        name: The target name for the Starlark build setting flag.
        fragment_field: The field name in ctx.fragments.proto (e.g. "strict_proto_deps"), if any.
        build_setting_default: Fallback default boolean value if neither Starlark nor fragment is set.
        values: Dict mapping booleans (True/False) to lists of accepted string flag values.
            Defaults to {True: ["true", "TRUE", "1"], False: ["false", "FALSE", "0"]}.
        **kwargs: Additional rule arguments (such as `scope`).
    """
    if values == None:
        values = {
            True: ["true", "TRUE", "1"],
            False: ["false", "FALSE", "0"],
        }

    true_vals = values.get(True, [])
    false_vals = values.get(False, [])

    starlark_true_vals = [v for v in true_vals if v != "default"]
    starlark_false_vals = [v for v in false_vals if v != "default"]

    _compat_bool_rule(
        name = name,
        build_setting_default = "default",
        fragment_field = fragment_field,
        true_values = starlark_true_vals,
        false_values = starlark_false_vals,
        default_value = build_setting_default,
        **kwargs
    )

def _compat_string_list_rule_impl(ctx):
    starlark_val = ctx.build_setting_value
    if starlark_val:
        return [BuildSettingInfo(value = starlark_val)]
    if hasattr(ctx.fragments, "proto") and ctx.attr.fragment_field:
        val = getattr(ctx.fragments.proto, ctx.attr.fragment_field)
        return [BuildSettingInfo(value = val)]
    return [BuildSettingInfo(value = ctx.attr.default_value)]

_compat_string_list_rule = rule(
    implementation = _compat_string_list_rule_impl,
    build_setting = config.string_list(flag = True, repeatable = True),
    fragments = ["proto"],
    attrs = {
        "fragment_field": attr.string(),
        "default_value": attr.string_list(default = []),
        "scope": attr.string(),
    },
)

def compat_string_list_flag(
        *,
        name,
        fragment_field = None,
        build_setting_default = None,
        **kwargs):
    """Creates a custom string-list build setting reconciling Starlark/fragments.

    Args:
        name: The target name for the Starlark build setting flag.
        fragment_field: The field name in ctx.fragments.proto (e.g. "experimental_protoc_opts"), if any.
        build_setting_default: Fallback default string list value if neither Starlark nor fragment is set.
        **kwargs: Additional rule arguments (such as `scope`).
    """
    default_vals = build_setting_default if build_setting_default != None else []

    _compat_string_list_rule(
        name = name,
        build_setting_default = default_vals,
        fragment_field = fragment_field,
        default_value = default_vals,
        **kwargs
    )
