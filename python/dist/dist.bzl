"""Rules to create python distribution files and properly name them"""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@system_python//:version.bzl", "SYSTEM_PYTHON_VERSION")

def _get_suffix(limited_api, python_version, cpu):
    """Computes an ABI version tag for an extension module per PEP 3149."""
    if "win32" in cpu or "win64" in cpu:
        if limited_api:
            return ".pyd"
        if "win32" in cpu:
            abi = "win32"
        elif "win64" in cpu:
            abi = "win_amd64"
        else:
            fail("Unsupported CPU: " + cpu)
        return ".cp{}-{}.{}".format(python_version, abi, "pyd")

    if python_version == "system":
        python_version = SYSTEM_PYTHON_VERSION
        if int(python_version) < 38:
            python_version += "m"
        abis = {
            "darwin_arm64": "darwin",
            "darwin_x86_64": "darwin",
            "darwin": "darwin",
            "osx-x86_64": "darwin",
            "osx-aarch_64": "darwin",
            "linux-aarch_64": "aarch64-linux-gnu",
            "linux-x86_64": "x86_64-linux-gnu",
            "k8": "x86_64-linux-gnu",
        }

        return ".cpython-{}-{}.{}".format(
            python_version,
            abis[cpu],
            "so" if limited_api else "abi3.so",
        )
    elif limited_api:
        return ".abi3.so"

    fail("Unsupported combination of flags")

def _declare_module_file(ctx, module_name, python_version, limited_api):
    """Declares an output file for a Python module with this name, version, and limited api."""
    base_filename = module_name.replace(".", "/")
    suffix = _get_suffix(
        python_version = python_version,
        limited_api = limited_api,
        cpu = ctx.var["TARGET_CPU"],
    )
    filename = base_filename + suffix
    return ctx.actions.declare_file(filename)

# --------------------------------------------------------------------------------------------------
# py_dist_module()
#
# Creates a Python binary extension module that is ready for distribution.
#
#   py_dist_module(
#       name = "message_mod",
#       extension = "//python:_message_binary",
#       module_name = "google._upb._message",
#   )
#
# In the simple case, this simply involves copying the input file to the proper filename for
# our current configuration (module_name, cpu, python_version, limited_abi).
#
# For multiarch platforms (osx-universal2), we must combine binaries for multiple architectures
# into a single output binary using the "llvm-lipo" tool.  A config transition depends on multiple
# architectures to get us the input files we need.

def _py_multiarch_transition_impl(settings, attr):
    if settings["//command_line_option:cpu"] == "osx-universal2":
        return [{"//command_line_option:cpu": cpu} for cpu in ["osx-aarch_64", "osx-x86_64"]]
    else:
        return settings

_py_multiarch_transition = transition(
    implementation = _py_multiarch_transition_impl,
    inputs = ["//command_line_option:cpu"],
    outputs = ["//command_line_option:cpu"],
)

def _py_dist_module_impl(ctx):
    output_file = _declare_module_file(
        ctx = ctx,
        module_name = ctx.attr.module_name,
        python_version = ctx.attr._python_version[BuildSettingInfo].value,
        limited_api = ctx.attr._limited_api[BuildSettingInfo].value,
    )
    if len(ctx.attr.extension) == 1:
        src = ctx.attr.extension[0][DefaultInfo].files.to_list()[0]
        ctx.actions.run(
            executable = "cp",
            arguments = [src.path, output_file.path],
            inputs = [src],
            outputs = [output_file],
        )
        return [
            DefaultInfo(files = depset([output_file])),
        ]
    else:
        srcs = [mod[DefaultInfo].files.to_list()[0] for mod in ctx.attr.extension]
        ctx.actions.run(
            executable = "/usr/local/bin/llvm-lipo",
            arguments = ["-create", "-output", output_file.path] + [src.path for src in srcs],
            inputs = srcs,
            outputs = [output_file],
        )
        return [
            DefaultInfo(files = depset([output_file])),
        ]

py_dist_module = rule(
    output_to_genfiles = True,
    implementation = _py_dist_module_impl,
    attrs = {
        "module_name": attr.string(mandatory = True),
        "extension": attr.label(
            mandatory = True,
            cfg = _py_multiarch_transition,
        ),
        "_limited_api": attr.label(default = "//python:limited_api"),
        "_python_version": attr.label(default = "//python:python_version"),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)

# --------------------------------------------------------------------------------------------------
# py_dist()
#
# A rule that builds a collection of binary wheels, using transitions to depend on many different
# python versions and cpus.

def _py_dist_transition_impl(settings, attr):
    _ignore = (settings)  # @unused
    transitions = []

    for cpu, version in attr.limited_api_wheels.items():
        transitions.append({
            "//command_line_option:cpu": cpu,
            "//python:python_version": version,
            "//python:limited_api": True,
        })

    for version in attr.full_api_versions:
        for cpu in attr.full_api_cpus:
            transitions.append({
                "//command_line_option:cpu": cpu,
                "//python:python_version": version,
                "//python:limited_api": False,
            })

    return transitions

_py_dist_transition = transition(
    implementation = _py_dist_transition_impl,
    inputs = [],
    outputs = [
        "//command_line_option:cpu",
        "//python:python_version",
        "//python:limited_api",
    ],
)

def _py_dist_impl(ctx):
    binary_files = [dep[DefaultInfo].files for dep in ctx.attr.binary_wheel]
    pure_python_files = [ctx.attr.pure_python_wheel[DefaultInfo].files]
    return [
        DefaultInfo(files = depset(
            transitive = binary_files + pure_python_files,
        )),
    ]

py_dist = rule(
    implementation = _py_dist_impl,
    attrs = {
        "binary_wheel": attr.label(
            mandatory = True,
            cfg = _py_dist_transition,
        ),
        "pure_python_wheel": attr.label(mandatory = True),
        "limited_api_wheels": attr.string_dict(),
        "full_api_versions": attr.string_list(),
        "full_api_cpus": attr.string_list(),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)
