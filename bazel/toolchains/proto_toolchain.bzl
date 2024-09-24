# Protocol Buffers - Google's data interchange format
# Copyright 2024 Google Inc.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd
#
"""Macro wrapping the proto_toolchain implementation.

The macro additionally creates toolchain target when toolchain_type is given.
"""

load("//bazel/private:proto_toolchain_rule.bzl", _proto_toolchain_rule = "proto_toolchain")
load("//bazel/private:toolchain_helpers.bzl", "toolchains")

def proto_toolchain(*, name, proto_compiler, exec_compatible_with = []):
    """Creates a proto_toolchain and toolchain target for proto_library.

    Toolchain target is suffixed with "_toolchain".

    Args:
      name: name of the toolchain
      proto_compiler: (Label) of either proto compiler sources or prebuild binaries
      exec_compatible_with: ([constraints]) List of constraints the prebuild binary is compatible with.
    """
    _proto_toolchain_rule(name = name, proto_compiler = proto_compiler)

    native.toolchain(
        name = name + "_toolchain",
        toolchain_type = toolchains.PROTO_TOOLCHAIN,
        exec_compatible_with = exec_compatible_with,
        target_compatible_with = [],
        toolchain = name,
    )
