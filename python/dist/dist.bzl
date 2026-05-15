"""Rules to create python distribution files and properly name them"""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@system_python//:version.bzl", "SYSTEM_PYTHON_VERSION")

def _get_os_name(ctx):
    for name, label in ctx.attr._os_constraints.items():
        if ctx.target_platform_has_constraint(label[platform_common.ConstraintValueInfo]):
            return name
    fail("Protobuf: Unknown OS for target platform")

def _get_cpu_name(ctx):
    for name, label in ctx.attr._cpu_constraints.items():
        if ctx.target_platform_has_constraint(label[platform_common.ConstraintValueInfo]):
            return name
    fail("Protobuf: Unknown CPU for target platform")

def _get_suffix(ctx, limited_api, python_version):
    """Computes an ABI version tag for an extension module per PEP 3149."""
    os = _get_os_name(ctx)
    cpu = _get_cpu_name(ctx)
    if os == "windows":
        if limited_api:
            return ".pyd"
        if cpu == "x86_32":
            abi = "win32"
        elif cpu == "x86_64":
            abi = "win_amd64"
        else:
            fail("Unsupported CPU: " + cpu)
        return ".cp{}-{}.{}".format(python_version, abi, "pyd")

    if python_version == "system":
        python_version = SYSTEM_PYTHON_VERSION
        if int(python_version) < 38:
            python_version += "m"

        if os == "osx":
            abi = "darwin"
        elif os == "linux":
            if cpu == "x86_64":
                abi = "x86_64-linux-gnu"
            elif cpu == "aarch64":
                abi = "aarch64-linux-gnu"
            elif cpu == "s390x":
                abi = "s390x-linux-gnu"
            else:
                fail("Unsupported CPU: " + cpu)
        else:
            fail("Unsupported OS: " + os)

        return ".cpython-{}-{}.{}".format(
            python_version,
            abi,
            "so" if limited_api else "abi3.so",
        )
    elif limited_api:
        return ".abi3.so"

    fail("Unsupported combination of flags")

def _declare_module_file(ctx, module_name, python_version, limited_api):
    """Declares an output file for a Python module with this name, version, and limited api."""
    base_filename = module_name.replace(".", "/")
    suffix = _get_suffix(
        ctx = ctx,
        python_version = python_version,
        limited_api = limited_api,
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
    if len(settings["//command_line_option:platforms"]) == 1 and settings["//command_line_option:platforms"][0] == Label("//build_defs:osx-universal2"):
        ret = [
            {"//command_line_option:platforms": platform}
            for platform in [
                "//build_defs:osx-aarch_64",
                "//build_defs:osx-x86_64",
            ]
        ]
        return ret
    else:
        return settings

_py_multiarch_transition = transition(
    implementation = _py_multiarch_transition_impl,
    inputs = ["//command_line_option:platforms"],
    outputs = ["//command_line_option:platforms"],
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
        "_os_constraints": attr.string_keyed_label_dict(
            default = {
                "osx": "@platforms//os:osx",
                "windows": "@platforms//os:windows",
                "linux": "@platforms//os:linux",
            },
        ),
        "_cpu_constraints": attr.string_keyed_label_dict(
            default = {
                "aarch64": "@platforms//cpu:aarch64",
                "x86_64": "@platforms//cpu:x86_64",
                "x86_32": "@platforms//cpu:x86_32",
                "s390x": "@platforms//cpu:s390x",
                "ppc64le": "@platforms//cpu:ppc64le",
            },
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

    for platform, version in attr.limited_api_platforms.items():
        transitions.append({
            "//command_line_option:platforms": platform,
            "//python:python_version": version,
            "//python:limited_api": True,
        })

    for version in attr.full_api_versions:
        for platform in attr.full_api_platforms:
            transitions.append({
                "//command_line_option:platforms": platform,
                "//python:python_version": version,
                "//python:limited_api": False,
            })

    return transitions

_py_dist_transition = transition(
    implementation = _py_dist_transition_impl,
    inputs = [],
    outputs = [
        "//command_line_option:platforms",
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
        "limited_api_platforms": attr.string_dict(),
        "full_api_versions": attr.string_list(),
        "full_api_platforms": attr.string_list(),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)
