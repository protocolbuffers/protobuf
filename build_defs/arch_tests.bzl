"""Generated unittests to verify that a binary is built for the expected architecture."""

load("//build_defs:internal_shell.bzl", "inline_sh_test")

def _arch_test_impl(
        name,
        platform_name,
        file_platform,
        config_setting_label,
        bazel_binaries = [],
        system_binaries = [],
        **kwargs):
    """Bazel rule to verify that a Bazel or system binary is built for the aarch64 architecture.

    Args:
      name: the name of the test.
      platform_name: a diagnostic name for this architecture.
      file_platform: the expected output of `file`.
      config_setting_label: the label of the config setting that defines this architecture.
      bazel_binaries: a set of binary targets to inspect.
      system_binaries: a set of paths to system executables to inspect.
      **kwargs: other keyword arguments that are passed to the test.
    """

    inline_sh_test(
        name = name,
        srcs = bazel_binaries,
        cmd = """
          for binary in "%s"; do
            (file -L $$binary | grep -q "%s") \\
                || (echo "Test binary is not an %s binary: "; file -L $$binary; exit 1)
          done
        """ % (
            " ".join(["$(rootpaths %s)" % b for b in bazel_binaries] + system_binaries),
            file_platform,
            platform_name,
        ),
        target_compatible_with = select({
            config_setting_label: [],
            "//conditions:default": ["@platforms//:incompatible"],
        }),
        **kwargs
    )

def aarch64_test(**kwargs):
    _arch_test_impl(
        platform_name = "aarch64",
        file_platform = "ELF 64-bit LSB.* ARM aarch64",
        config_setting_label = "//build_defs:is_linux_aarch64",
        **kwargs
    )

def x86_64_test(**kwargs):
    _arch_test_impl(
        platform_name = "x86_64",
        file_platform = "ELF 64-bit LSB.* x86-64",
        config_setting_label = "//build_defs:is_linux_x86_64",
        **kwargs
    )
