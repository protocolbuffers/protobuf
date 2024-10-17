# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""proto_lang_toolchain rule"""

load("@proto_bazel_features//:features.bzl", "bazel_features")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/private:proto_lang_toolchain_rule.bzl", _proto_lang_toolchain_rule = "proto_lang_toolchain")

def proto_lang_toolchain(*, name, toolchain_type = None, exec_compatible_with = [], target_compatible_with = [], **attrs):
    """Creates a proto_lang_toolchain and corresponding toolchain target.

    Toolchain target is only created when toolchain_type is set.

    https://docs.bazel.build/versions/master/be/protocol-buffer.html#proto_lang_toolchain

    Args:

      name: name of the toolchain
      toolchain_type: The toolchain type
      exec_compatible_with: ([constraints]) List of constraints the prebuild binaries is compatible with.
      target_compatible_with: ([constraints]) List of constraints the target libraries are compatible with.
      **attrs: Rule attributes
    """

    if getattr(proto_common, "INCOMPATIBLE_PASS_TOOLCHAIN_TYPE", False):
        attrs["toolchain_type"] = toolchain_type

    # This condition causes Starlark rules to be used only on Bazel >=7.0.0
    if bazel_features.proto.starlark_proto_info:
        _proto_lang_toolchain_rule(name = name, **attrs)
    else:
        # On older Bazel versions keep using native rules, so that mismatch in ProtoInfo doesn't happen
        native.proto_lang_toolchain(name = name, **attrs)

    if toolchain_type:
        native.toolchain(
            name = name + "_toolchain",
            toolchain_type = toolchain_type,
            exec_compatible_with = exec_compatible_with,
            target_compatible_with = target_compatible_with,
            toolchain = name,
        )
