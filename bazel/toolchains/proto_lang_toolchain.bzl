"""proto_lang_toolchain rule"""

load("//bazel/common:proto_common.bzl", "proto_common")

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

    # buildifier: disable=native-proto
    native.proto_lang_toolchain(name = name, **attrs)

    if toolchain_type:
        native.toolchain(
            name = name + "_toolchain",
            toolchain_type = toolchain_type,
            exec_compatible_with = exec_compatible_with,
            target_compatible_with = target_compatible_with,
            toolchain = name,
        )
