"""
This module exports the pkg_cross_compiled_binaries rule. This rule is used to create a pkg_filegroup
that contains the cross compiled binaries for each cpu.
"""

load("@rules_pkg//pkg:mappings.bzl", "pkg_attributes", "pkg_filegroup", "pkg_files")

def _cpu_transition_impl(settings, attr):
    _ignore = (settings)  # @unused
    return [{"//command_line_option:cpu": attr.cpu}]

_cpu_transition = transition(
    implementation = _cpu_transition_impl,
    inputs = [],
    outputs = [
        "//command_line_option:cpu",
    ],
)

def _cross_compiled_binary_impl(ctx):
    target = ctx.attr.target

    default_info = target[0][DefaultInfo]
    files = default_info.files
    runfiles = default_info.default_runfiles

    files = depset(transitive = [files])

    return [
        DefaultInfo(
            files = files,
            runfiles = runfiles,
        ),
    ]

_cross_compiled_binary = rule(
    implementation = _cross_compiled_binary_impl,
    attrs = {
        "target": attr.label(
            mandatory = True,
            cfg = _cpu_transition,
        ),
        "cpu": attr.string(),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)

def pkg_cross_compiled_binaries(name, cpus, targets, prefix, tags, visibility = None):
    """Creates a pkg_filegroup that contains the cross compiled binaries for each cpu.

    This rule is used to create a pkg_filegroup that contains the cross compiled binaries for each
    cpu. The binaries are created by an aspect that changes the cpu configuration for each target.
    The targets are placed into a directory that is named after the cpu.

    Args:
      name: The name of the pkg_filegroup.
      cpus: The cpus to cross compile for.
      targets: The targets to cross compile.
      prefix: The prefix to add to the pkg_filegroup.
      tags: The tags to add to the pkg_filegroup.
      visibility: The visibility of the pkg_filegroup.
    """

    filegroups = []
    for cpu in cpus:
        compiled_targets = []
        for target in targets:
            target_name = target.split(":")[1]
            bin_target_name = name + "_" + cpu + "_" + target_name
            _cross_compiled_binary(
                name = bin_target_name,
                cpu = cpu,
                target = target,
            )
            compiled_targets.append(Label(":" + bin_target_name))
        files_name = name + "_" + cpu + "_src"
        filegroup_name = files_name + "_dir"
        filegroups.append(Label(":" + filegroup_name))
        pkg_files(
            name = files_name,
            srcs = compiled_targets,
            attributes = pkg_attributes(
                mode = "0755",
            ),
        )
        pkg_filegroup(
            name = filegroup_name,
            srcs = [Label(":" + files_name)],
            prefix = cpu,
        )
    pkg_filegroup(
        name = name,
        srcs = filegroups,
        prefix = prefix,
        tags = tags,
        visibility = visibility,
    )
    return
