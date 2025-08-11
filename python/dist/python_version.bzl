# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Fetches python version from rules_python toolchain"""

def _python_version_impl(ctx):
    toolchain = ctx.toolchains["@rules_python//python:toolchain_type"]
    info = toolchain.py3_runtime.interpreter_version_info
    python_version = str(info.major) + "." + str(info.minor) + "." + str(info.micro)
    output_file = ctx.actions.declare_file(ctx.attr.name + ".txt")
    ctx.actions.write(output = output_file, content = python_version)
    return [DefaultInfo(files = depset([output_file]))]

python_version = rule(
    implementation = _python_version_impl,
    attrs = {},
    toolchains = ["@rules_python//python:toolchain_type"],
)
