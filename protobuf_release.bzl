# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""
Generates package naming variables for use with rules_pkg.
"""

load("@rules_pkg//pkg:providers.bzl", "PackageVariablesInfo")
load("//toolchain:platforms.bzl", "PROTOBUF_PLATFORMS")
load(":protobuf_version.bzl", "PROTOC_VERSION")

def _package_naming_impl(ctx):
    return PackageVariablesInfo(values = {
        "version": PROTOC_VERSION,
        "platform": ctx.attr.platform,
    })

_package_naming = rule(
    implementation = _package_naming_impl,
    attrs = {
        "platform": attr.string(mandatory = True),
    },
)

def package_naming(name):
    for platform, meta in PROTOBUF_PLATFORMS.items():
        native.config_setting(
            name = name + "_is_" + platform.replace("-", "_"),
            constraint_values = meta["compatible_with"],
            visibility = ["//visibility:private"],
        )
    _package_naming(
        name = name,
        platform = select({
            ":" + name + "_is_" + platform.replace("-", "_"): platform
            for platform in PROTOBUF_PLATFORMS.keys()
        }),
    )
