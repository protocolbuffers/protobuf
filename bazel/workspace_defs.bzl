# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Repository rule for using Python 3.x headers from the system."""

_build_file = """
load("@bazel_tools//tools/python:toolchain.bzl", "py_runtime_pair")
cc_library(
   name = "python_headers",
   hdrs = glob(["python/**/*.h"]),
   includes = ["python"],
   visibility = ["//visibility:public"],
)

py_runtime(
    name = "py3_runtime",
    interpreter_path = "%s",
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

_build_defs_file = """
EXT_SUFFIX = "%s"
"""

def _get_config_var(repository_ctx, name):
  py_program = "import sysconfig; print(sysconfig.get_config_var('%s'), end='')"
  result = repository_ctx.execute(["python3", "-c", py_program % (name)])
  if result.return_code != 0:
    fail("No python3 executable available on the system")
  return result.stdout

def _python_headers_impl(repository_ctx):
  path = _get_config_var(repository_ctx, "INCLUDEPY")
  ext_suffix = _get_config_var(repository_ctx, "EXT_SUFFIX")
  repository_ctx.symlink(path, "python")
  python3 = repository_ctx.which("python3")
  repository_ctx.file("BUILD.bazel", _build_file % python3)
  repository_ctx.file("build_defs.bzl", _build_defs_file % ext_suffix)

# The system_python() repository rule exposes Python headers from the system.
#
# In WORKSPACE:
#   system_python(
#       name = "system_python_repo",
#   )
#
# This repository exposes a single rule that you can depend on from BUILD:
#   cc_library(
#     name = "foobar",
#     srcs = ["foobar.cc"],
#     deps = ["@system_python_repo//:python_headers"],
#   )
#
# The headers should correspond to the version of python obtained by running
# the `python3` command on the system.
system_python = repository_rule(
    implementation = _python_headers_impl,
    local = True,
)
