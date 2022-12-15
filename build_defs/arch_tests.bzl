"""Generated unittests to verify that a binary is built for the expected architecture."""

load("//build_defs:internal_shell.bzl", "inline_sh_test")

_ARCHITECTURE_MATCHER = {
    "aarch64": "ELF 64-bit LSB.* ARM aarch64",
    "x86_64": "ELF 64-bit LSB.* x86_64",
}

def binary_architecture_test(
        name,
        architecture,
        binaries = [],
        system_binaries = [],
        target_compatible_with = [],
        tags = [],
        **kwargs):
    """Bazel rule to verify that binaries are built for a specific architecture.

    This is used as a sanity test of our test infrastructure, to make sure that we're
    actually cross-compiling for the correct platform.

    Args:
      name: the name of the test.
      architecture: an identifier for the architecture being tested.
      binaries: a set of binary targets to inspect.
      system_binaries: a set of system executables to inspect.
      target_compatible_with: config settings this test is compatible with.
      tags: tags for filtering these tests.
      **kwargs: other keyword arguments that are passed to the test.
    """
    if architecture not in _ARCHITECTURE_MATCHER:
        fail("Unknown architecture: %s", architecture)

    inline_sh_test(
        name = name,
        tools = binaries,
        cmd = """
          for binary in "%s"; do
            (file -L $$binary | grep -q "%s") \
                || (echo "Test binary is not an %s binary: "; file -L $$binary; exit 1)
          done
        """ % (
            " ".join(["$(rootpaths %s)" % b for b in binaries] + system_binaries),
            _ARCHITECTURE_MATCHER[architecture],
            architecture,
        ),
        target_compatible_with = target_compatible_with,
        tags = tags + [
            "binary_architecture_test",
            "%s_test" % architecture,
        ],
        **kwargs
    )

def aarch64_test(**kwargs):
    _arch_test_impl(
        platform = "aarch64",
        file_platform = "ELF 64-bit LSB.* ARM aarch64",
        **kwargs
    )

def x86_64_test(**kwargs):
    _arch_test_impl(
        platform = "x86_64",
        file_platform = "ELF 64-bit LSB.*, ARM x86_64",
        **kwargs
    )
