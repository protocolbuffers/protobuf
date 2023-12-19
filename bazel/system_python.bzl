# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Repository rule for using Python 3.x headers from the system."""

# Mock out rules_python's pip.bzl for cases where no system python is found.
_mock_pip = """
def _pip_install_impl(repository_ctx):
    repository_ctx.file("BUILD.bazel", '''
py_library(
    name = "noop",
    visibility = ["//visibility:public"],
)
''')
    repository_ctx.file("requirements.bzl", '''
def install_deps(*args, **kwargs):
    print("WARNING: could not install pip dependencies")

def requirement(*args, **kwargs):
    return "@{}//:noop"
'''.format(repository_ctx.attr.name))
pip_install = repository_rule(
    implementation = _pip_install_impl,
    attrs = {
        "requirements": attr.string(),
        "requirements_overrides": attr.string_dict(),
        "python_interpreter_target": attr.string(),
    },
)
pip_parse = pip_install
"""

# Alias rules_python's pip.bzl for cases where a system python is found.
_alias_pip = """
load("@rules_python//python:pip.bzl", _pip_install = "pip_install", _pip_parse = "pip_parse")

def _get_requirements(requirements, requirements_overrides):
    for version, override in requirements_overrides.items():
        if version in "{python_version}":
            requirements = override
            break
    return requirements

def pip_install(requirements, requirements_overrides={{}}, **kwargs):
    _pip_install(
        python_interpreter_target = "@{repo}//:interpreter",
        requirements = _get_requirements(requirements, requirements_overrides),
        **kwargs,
    )
def pip_parse(requirements, requirements_overrides={{}}, **kwargs):
    _pip_parse(
        python_interpreter_target = "@{repo}//:interpreter",
        requirements = _get_requirements(requirements, requirements_overrides),
        **kwargs,
    )
"""

_mock_fuzzing_py = """
def fuzzing_py_install_deps():
    print("WARNING: could not install fuzzing_py dependencies")
"""

# Alias rules_fuzzing's requirements.bzl for cases where a system python is found.
_alias_fuzzing_py = """
load("@fuzzing_py_deps//:requirements.bzl", _fuzzing_py_install_deps = "install_deps")

def fuzzing_py_install_deps():
    _fuzzing_py_install_deps()
"""

_build_file = """
load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:common_settings.bzl", "string_flag")
load("@bazel_tools//tools/python:toolchain.bzl", "py_runtime_pair")

cc_library(
   name = "python_headers",
   hdrs = glob(["python/**/*.h"], allow_empty = True),
   includes = ["python"],
   visibility = ["//visibility:public"],
)

string_flag(
    name = "internal_python_support",
    build_setting_default = "{support}",
    values = [
        "None",
        "Supported",
        "Unsupported",
    ]
)

config_setting(
    name = "none",
    flag_values = {{
        ":internal_python_support": "None",
    }},
    visibility = ["//visibility:public"],
)

config_setting(
    name = "supported",
    flag_values = {{
        ":internal_python_support": "Supported",
    }},
    visibility = ["//visibility:public"],
)

config_setting(
    name = "unsupported",
    flag_values = {{
        ":internal_python_support": "Unsupported",
    }},
    visibility = ["//visibility:public"],
)

selects.config_setting_group(
    name = "exists",
    match_any = [":supported", ":unsupported"],
    visibility = ["//visibility:public"],
)

sh_binary(
    name = "interpreter",
    srcs = ["interpreter"],
    visibility = ["//visibility:public"],
)

py_runtime(
    name = "py3_runtime",
    interpreter_path = "{interpreter}",
    python_version = "PY3",
)

py_runtime_pair(
    name = "runtime_pair",
    py3_runtime = ":py3_runtime",
)

toolchain(
    name = "python_toolchain",
    toolchain = ":runtime_pair",
    toolchain_type = "@rules_python//python:toolchain_type",
)
"""

_register = """
def register_system_python():
    native.register_toolchains("@{}//:python_toolchain")
"""

_mock_register = """
def register_system_python():
    pass
"""

def _get_python_version(repository_ctx):
    py_program = "import sys; print(str(sys.version_info.major) + '.' + str(sys.version_info.minor) + '.' + str(sys.version_info.micro))"
    result = repository_ctx.execute(["python3", "-c", py_program])
    return (result.stdout).strip().split(".")

def _get_python_path(repository_ctx):
    py_program = "import sysconfig; print(sysconfig.get_config_var('%s'), end='')"
    result = repository_ctx.execute(["python3", "-c", py_program % ("INCLUDEPY")])
    if result.return_code != 0:
        return None
    return result.stdout

def _populate_package(ctx, path, python3, python_version):
    ctx.symlink(path, "python")
    supported = True
    for idx, v in enumerate(ctx.attr.minimum_python_version.split(".")):
        if int(python_version[idx]) < int(v):
            supported = False
            break
    if "win" in ctx.os.name:
        # buildifier: disable=print
        print("WARNING: python is not supported on Windows")
        supported = False

    build_file = _build_file.format(
        interpreter = python3,
        support = "Supported" if supported else "Unsupported",
    )

    ctx.file("interpreter", "#!/bin/sh\nexec {} \"$@\"".format(python3))
    ctx.file("BUILD.bazel", build_file)
    ctx.file("version.bzl", "SYSTEM_PYTHON_VERSION = '{}{}'".format(python_version[0], python_version[1]))
    ctx.file("register.bzl", _register.format(ctx.attr.name))
    if supported:
        ctx.file("pip.bzl", _alias_pip.format(
            python_version = ".".join(python_version),
            repo = ctx.attr.name,
        ))
        ctx.file("fuzzing_py.bzl", _alias_fuzzing_py)
    else:
        # Dependencies are unlikely to be satisfiable for unsupported versions of python.
        ctx.file("pip.bzl", _mock_pip)
        ctx.file("fuzzing_py.bzl", _mock_fuzzing_py)

def _populate_empty_package(ctx):
    # Mock out all the entrypoints we need to run from WORKSPACE.  Targets that
    # actually need python should use `target_compatible_with` and the generated
    # @system_python//:exists or @system_python//:supported constraints.
    ctx.file(
        "BUILD.bazel",
        _build_file.format(
            interpreter = "",
            support = "None",
        ),
    )
    ctx.file("version.bzl", "SYSTEM_PYTHON_VERSION = 'None'")
    ctx.file("register.bzl", _mock_register)
    ctx.file("pip.bzl", _mock_pip)
    ctx.file("fuzzing_py.bzl", _mock_fuzzing_py)

def _system_python_impl(repository_ctx):
    path = _get_python_path(repository_ctx)
    python3 = repository_ctx.which("python3")
    python_version = _get_python_version(repository_ctx)

    if path and python_version[0] == "3":
        _populate_package(repository_ctx, path, python3, python_version)
    else:
        # buildifier: disable=print
        print("WARNING: no system python available, builds against system python will fail")
        _populate_empty_package(repository_ctx)

# The system_python() repository rule exposes information from the version of python installed in the current system.
#
# In WORKSPACE:
#   system_python(
#       name = "system_python_repo",
#       minimum_python_version = "3.7",
#   )
#
# This repository exposes some repository rules for configuring python in Bazel.  The python toolchain
# *must* be registered in your WORKSPACE:
#   load("@system_python_repo//:register.bzl", "register_system_python")
#   register_system_python()
#
# Pip dependencies can optionally be specified using a wrapper around rules_python's repository rules:
#   load("@system_python//:pip.bzl", "pip_install")
#   pip_install(
#       name="pip_deps",
#       requirements = "@com_google_protobuf//python:requirements.txt",
#   )
# An optional argument `requirements_overrides` takes a dictionary mapping python versions to alternate
# requirements files.  This works around the requirement for fully pinned dependencies in python_rules.
#
# Four config settings are exposed from this repository to help declare target compatibility in Bazel.
# For example, `@system_python_repo//:exists` will be true if a system python version has been found.
# The `none` setting will be true only if no python version was found, and `supported`/`unsupported`
# correspond to whether or not the system version is compatible with `minimum_python_version`.
#
# This repository also exposes a header rule that you can depend on from BUILD files:
#   cc_library(
#     name = "foobar",
#     srcs = ["foobar.cc"],
#     deps = ["@system_python_repo//:python_headers"],
#   )
#
# The headers should correspond to the version of python obtained by running
# the `python3` command on the system.
system_python = repository_rule(
    implementation = _system_python_impl,
    local = True,
    attrs = {
        "minimum_python_version": attr.string(default = "3.7"),
    },
)
