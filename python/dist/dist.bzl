"""Rules to create python distribution files and properly name them"""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")
load("@system_python//:version.bzl", "SYSTEM_PYTHON_VERSION")

def _get_suffix(limited_api, python_version, cpu):
    suffix = "pyd" if ("win" in cpu) else "so"

    if limited_api == True:
        if "win" not in cpu:
            suffix = "abi3." + suffix
        return "." + suffix

    if "win32" in cpu or "win64" in cpu:
        if "win32" in cpu:
            abi = "win32"
        elif "win64" in cpu:
            abi = "win_amd64"
        else:
            fail("Unsupported CPU: " + cpu)
        return ".cp{}-{}.{}".format(python_version, abi, suffix)

    if python_version == "system":
        python_version = SYSTEM_PYTHON_VERSION
        if int(python_version) < 38:
            python_version += "m"
        abis = {
            "darwin": "darwin",
            "osx-x86_64": "darwin",
            "osx-aarch_64": "darwin",
            "linux-aarch_64": "aarch64-linux-gnu",
            "linux-x86_64": "x86_64-linux-gnu",
            "k8": "x86_64-linux-gnu",
        }

        return ".cpython-{}-{}.{}".format(python_version, abis[cpu], suffix)

    fail("Unsupported combination of flags")

def _py_dist_module_impl(ctx):
    base_filename = ctx.attr.module_name.replace(".", "/")
    suffix = _get_suffix(
        limited_api = ctx.attr._limited_api[BuildSettingInfo].value,
        python_version = ctx.attr._python_version[BuildSettingInfo].value,
        cpu = ctx.var["TARGET_CPU"],
    )
    filename = base_filename + suffix
    file = ctx.actions.declare_file(filename)
    src = ctx.attr.extension[DefaultInfo].files.to_list()[0]
    ctx.actions.run(
        executable = "cp",
        arguments = [src.path, file.path],
        inputs = [src],
        outputs = [file],
    )
    return [
        DefaultInfo(files = depset([file])),
    ]

_py_dist_module_rule = rule(
    output_to_genfiles = True,
    implementation = _py_dist_module_impl,
    fragments = ["cpp"],
    attrs = {
        "module_name": attr.string(mandatory = True),
        "extension": attr.label(
            mandatory = True,
            providers = [CcInfo],
        ),
        "_limited_api": attr.label(default = "//python:limited_api"),
        "_python_version": attr.label(default = "//python:python_version"),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
    },
)

def py_dist_module(name, module_name, extension):
    file_rule = name + "_file"
    _py_dist_module_rule(
        name = file_rule,
        module_name = module_name,
        extension = extension,
    )

    # TODO(haberman): needed?
    native.py_library(
        name = name,
        data = [":" + file_rule],
        imports = ["."],
    )

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
    return [
        DefaultInfo(files = depset(
            transitive = [dep[DefaultInfo].files for dep in ctx.attr.binary_wheel],
        )),
    ]

py_dist = rule(
    implementation = _py_dist_impl,
    attrs = {
        "binary_wheel": attr.label(
            mandatory = True,
            cfg = _py_dist_transition,
        ),
        "limited_api_wheels": attr.string_dict(),
        "full_api_versions": attr.string_list(),
        "full_api_cpus": attr.string_list(),
        "_allowlist_function_transition": attr.label(
            default = "@bazel_tools//tools/allowlists/function_transition_allowlist",
        ),
    },
)
