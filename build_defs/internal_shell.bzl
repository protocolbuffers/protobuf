"""
Internal tools to migrate shell commands to Bazel as an intermediate step
to wider Bazelification.
"""

def inline_sh_binary(
        name,
        srcs = [],
        tools = [],
        deps = [],
        cmd = "",
        **kwargs):
    """Bazel rule to wrap up an inline bash script in a binary.

    This is most useful as a stop-gap solution for migrating off Autotools.
    These binaries are likely to be non-hermetic, with implicit system
    dependencies.

    NOTE: the rule is only an internal workaround. The interface may change and
    the rule may be removed when everything is properly "Bazelified".

    Args:
      name: the name of the inline_sh_binary.
      srcs: the files used directly by the script.
      tools: the executable tools used directly by the script.  Any target used
        with rootpath/execpath/location must be declared here or in `srcs`.
      deps: a list of dependency labels that are required to run this binary.
      cmd: the inline sh command to run.
      **kwargs: other keyword arguments that are passed to sh_binary.
    """

    native.genrule(
        name = name + "_genrule",
        srcs = srcs,
        exec_tools = tools,
        outs = [name + ".sh"],
        cmd = "cat <<'EOF' >$(OUTS)\n#!/bin/bash -exu\n%s\nEOF\n" % cmd,
        visibility = ["//visibility:private"],
        tags = kwargs["tags"] if "tags" in kwargs else None,
        target_compatible_with = kwargs["target_compatible_with"] if "target_compatible_with" in kwargs else None,
        testonly = kwargs["testonly"] if "testonly" in kwargs else None,
    )

    native.sh_binary(
        name = name,
        srcs = [name + "_genrule"],
        data = srcs + tools + deps,
        **kwargs
    )

def inline_sh_test(
        name,
        srcs = [],
        tools = [],
        deps = [],
        cmd = "",
        **kwargs):
    """Bazel rule to wrap up an inline bash script in a test.

    This is most useful as a stop-gap solution for migrating off Autotools.
    These tests are likely to be non-hermetic, with implicit system dependencies.

    NOTE: the rule is only an internal workaround. The interface may change and
    the rule may be removed when everything is properly "Bazelified".

    Args:
      name: the name of the inline_sh_binary.
      srcs: the files used directly by the script.
      tools: the executable tools used directly by the script.  Any target used
        with rootpath/execpath/location must be declared here or in `srcs`.
      deps: a list of dependency labels that are required to run this binary.
      cmd: the inline sh command to run.
      **kwargs: other keyword arguments that are passed to sh_binary.
          https://bazel.build/reference/be/common-definitions#common-attributes)
    """

    native.genrule(
        name = name + "_genrule",
        srcs = srcs,
        exec_tools = tools,
        outs = [name + ".sh"],
        cmd = "cat <<'EOF' >$(OUTS)\n#!/bin/bash -exu\n%s\nEOF\n" % cmd,
        visibility = ["//visibility:private"],
        tags = kwargs["tags"] if "tags" in kwargs else None,
        target_compatible_with = kwargs["target_compatible_with"] if "target_compatible_with" in kwargs else None,
        testonly = kwargs["testonly"] if "testonly" in kwargs else None,
    )

    native.sh_test(
        name = name,
        srcs = [name + "_genrule"],
        data = srcs + tools + deps,
        **kwargs
    )
