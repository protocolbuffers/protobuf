# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""Helper rules and macros for custom proto flags."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_java//java/common:java_info.bzl", "JavaInfo")
load("@rules_java//java/common:proguard_spec_info.bzl", "ProguardSpecInfo")
load("//bazel/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")
load("//bazel/private:native.bzl", "HAS_NATIVE_PROTO_FLAGS")

def _compat_bool_rule_impl(ctx):
    starlark_val = ctx.build_setting_value

    # 1. Starlark flag takes precedence if explicitly passed (not "default")
    if starlark_val != "default":
        if starlark_val in ctx.attr.true_values:
            return [BuildSettingInfo(value = True)]
        if starlark_val in ctx.attr.false_values:
            return [BuildSettingInfo(value = False)]
        allowed = ctx.attr.true_values + ctx.attr.false_values + ["default"]
        fail("Invalid value '%s' for flag %s. Allowed values are: %s" % (
            starlark_val,
            ctx.label,
            allowed,
        ))

    # 2. Native flag fallback (if HAS_NATIVE_PROTO_FLAGS and present on ctx.fragments.proto)
    if HAS_NATIVE_PROTO_FLAGS and hasattr(ctx.fragments, "proto") and ctx.attr.fragment_field:
        if hasattr(ctx.fragments.proto, ctx.attr.fragment_field):
            native_val = getattr(ctx.fragments.proto, ctx.attr.fragment_field)()
            if native_val == True or native_val in ctx.attr.true_values:
                return [BuildSettingInfo(value = True)]
            if native_val == False or native_val in ctx.attr.false_values:
                return [BuildSettingInfo(value = False)]

    # 3. Default fallback
    return [BuildSettingInfo(value = ctx.attr.default_value)]

_compat_bool_rule = rule(
    implementation = _compat_bool_rule_impl,
    build_setting = config.string(flag = True),
    fragments = ["proto"] if HAS_NATIVE_PROTO_FLAGS else [],
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

    # 1. Starlark flag takes precedence if explicitly passed (not empty)
    if starlark_val:
        return [BuildSettingInfo(value = starlark_val)]

    # 2. Native flag fallback (if HAS_NATIVE_PROTO_FLAGS and present on ctx.fragments.proto)
    if HAS_NATIVE_PROTO_FLAGS and hasattr(ctx.fragments, "proto") and ctx.attr.fragment_field:
        if hasattr(ctx.fragments.proto, ctx.attr.fragment_field):
            val = getattr(ctx.fragments.proto, ctx.attr.fragment_field)
            if type(val) == "list":
                return [BuildSettingInfo(value = val)]
            else:
                return [BuildSettingInfo(value = val())]

    # 3. Default fallback
    return [BuildSettingInfo(value = ctx.attr.default_value)]

_compat_string_list_rule = rule(
    implementation = _compat_string_list_rule_impl,
    build_setting = config.string_list(flag = True, repeatable = True),
    fragments = ["proto"] if HAS_NATIVE_PROTO_FLAGS else [],
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

def _forward_providers(ctx, label_val, target = None, extra_providers = []):
    if target == None:
        target = ctx.attr.default_value
    providers = [BuildSettingInfo(value = label_val)] + extra_providers
    if ProtoLangToolchainInfo in target:
        providers.append(target[ProtoLangToolchainInfo])
        if target[ProtoLangToolchainInfo].runtime and JavaInfo in target[ProtoLangToolchainInfo].runtime:
            providers.append(target[ProtoLangToolchainInfo].runtime[JavaInfo])
        if target[ProtoLangToolchainInfo].runtime and ProguardSpecInfo in target[ProtoLangToolchainInfo].runtime:
            providers.append(target[ProtoLangToolchainInfo].runtime[ProguardSpecInfo])

    if JavaInfo in target:
        providers.append(target[JavaInfo])

    if ProguardSpecInfo in target:
        providers.append(target[ProguardSpecInfo])

    if CcInfo in target:
        providers.append(target[CcInfo])

    return providers

def _compat_label_rule_impl(ctx):
    target = ctx.attr.default_value
    label_val = target.label

    starlark_val = ctx.build_setting_value

    # 1. Starlark flag takes precedence if explicitly passed (not "default")
    if starlark_val != "default":
        label_val = Label(starlark_val)

        # 2. Native flag fallback (if HAS_NATIVE_PROTO_FLAGS and _native_target present on ctx.attr)
    elif HAS_NATIVE_PROTO_FLAGS and hasattr(ctx.attr, "_native_target") and ctx.attr._native_target:
        native_target = ctx.attr._native_target
        if native_target.label != target.label:
            target = native_target
            label_val = native_target.label

    return _forward_providers(ctx, label_val, target = target)

def _compat_executable_label_rule_impl(ctx):
    target = ctx.attr.default_value
    label_val = target.label

    starlark_val = ctx.build_setting_value
    if starlark_val != "default":
        label_val = Label(starlark_val)
    elif HAS_NATIVE_PROTO_FLAGS and hasattr(ctx.attr, "_native_target") and ctx.attr._native_target:
        native_target = ctx.attr._native_target
        if native_target.label != target.label:
            target = native_target
            label_val = native_target.label

    extra_providers = []
    if DefaultInfo in target:
        def_info = target[DefaultInfo]
        if def_info.files_to_run and def_info.files_to_run.executable:
            orig_exec = def_info.files_to_run.executable
            symlink = ctx.actions.declare_file(orig_exec.basename)
            ctx.actions.symlink(
                output = symlink,
                target_file = orig_exec,
                is_executable = True,
            )
            extra_providers.append(DefaultInfo(
                files = depset([symlink]),
                runfiles = def_info.default_runfiles,
                executable = symlink,
            ))

    return _forward_providers(ctx, label_val, target = target, extra_providers = extra_providers)

_COMMON_LABEL_FLAG_ATTRS = {
    "fragment_field": attr.string(),
    "default_value": attr.label(allow_files = True),
    "runtime": attr.label(allow_files = True),
    "scope": attr.string(),
}

def _make_label_rule(fragment_name, executable = False):
    attrs = dict(_COMMON_LABEL_FLAG_ATTRS)
    if HAS_NATIVE_PROTO_FLAGS and fragment_name:
        attrs["_native_target"] = attr.label(
            default = configuration_field(fragment = "proto", name = fragment_name),
            allow_files = True,
        )
    return rule(
        implementation = _compat_executable_label_rule_impl if executable else _compat_label_rule_impl,
        build_setting = config.string(flag = True),
        executable = executable,
        fragments = ["proto"] if HAS_NATIVE_PROTO_FLAGS and fragment_name else [],
        attrs = attrs,
    )

_compat_label_rule_compiler = _make_label_rule("proto_compiler", executable = False)
_compat_executable_label_rule_compiler = _make_label_rule("proto_compiler", executable = True)
_compat_label_rule_cc = _make_label_rule("proto_toolchain_for_cc", executable = False)
_compat_label_rule_java = _make_label_rule("proto_toolchain_for_java", executable = False)
_compat_label_rule_javalite = _make_label_rule("proto_toolchain_for_java_lite", executable = False)
_compat_label_rule_default = _make_label_rule(None, executable = False)
_compat_executable_label_rule_default = _make_label_rule(None, executable = True)

_LABEL_RULES = {
    ("proto_compiler", False): _compat_label_rule_compiler,
    ("proto_compiler", True): _compat_executable_label_rule_compiler,
    ("proto_toolchain_for_cc", False): _compat_label_rule_cc,
    ("proto_toolchain_for_java", False): _compat_label_rule_java,
    ("proto_toolchain_for_javalite", False): _compat_label_rule_javalite,
    (None, False): _compat_label_rule_default,
    (None, True): _compat_executable_label_rule_default,
}

def compat_label_flag(
        *,
        name,
        fragment_field = None,
        build_setting_default = None,
        executable = False,
        **kwargs):
    """Creates a custom label build setting reconciling Starlark/fragments.

    Args:
        name: The target name for the Starlark build setting flag.
        fragment_field: The field name in ctx.fragments.proto, if any.
        build_setting_default: Fallback default label target.
        executable: Whether the label setting points to an executable target.
        **kwargs: Additional rule arguments (such as `scope`).
    """
    if fragment_field != None and (fragment_field, executable) not in _LABEL_RULES:
        fail("Unsupported fragment_field '%s' for compat_label_flag. Supported values are: %s" % (
            fragment_field,
            sorted([k[0] for k in _LABEL_RULES.keys() if k[0] != None]),
        ))

    rule_func = _LABEL_RULES.get((fragment_field, executable), _LABEL_RULES[(None, executable)])
    rule_func(
        name = name,
        build_setting_default = "default",
        fragment_field = fragment_field or "",
        default_value = build_setting_default,
        runtime = build_setting_default,
        **kwargs
    )
