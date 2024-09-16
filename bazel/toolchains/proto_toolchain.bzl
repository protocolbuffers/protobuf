"""Macro wrapping the proto_toolchain implementation.

The macro additionally creates toolchain target when toolchain_type is given.
"""

load("//bazel/private:proto_toolchain_rule.bzl", _proto_toolchain_rule = "proto_toolchain")

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
        toolchain_type = "@rules_proto//proto:toolchain_type",
        exec_compatible_with = exec_compatible_with,
        target_compatible_with = [],
        toolchain = name,
    )
