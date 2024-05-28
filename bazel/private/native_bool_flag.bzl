# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
"""
A helper rule that reads a native boolean flag.
"""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _impl(ctx):
    return [BuildSettingInfo(value = ctx.attr.value)]

_native_bool_flag_rule = rule(
    implementation = _impl,
    attrs = {"value": attr.bool()},
)

def native_bool_flag(*, name, flag, match_value = "true", result = True, **kwargs):
    _native_bool_flag_rule(
        name = name,
        value = select({
            name + "_setting": result,
            "//conditions:default": not result,
        }),
        **kwargs
    )

    native.config_setting(
        name = name + "_setting",
        values = {flag: match_value},
        visibility = ["//visibility:private"],
    )
