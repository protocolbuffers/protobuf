# Copyright (c) 2023, Google LLC
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

"""A wrapper around proto_common.compile() that fixes some surprising behaviors.

More info in: https://github.com/bazelbuild/bazel/issues/18263
"""

load("@rules_proto//proto:defs.bzl", "proto_common")

def output_dir(ctx, proto_info):
    """Returns the output directory where generated proto files will be placed.

    Args:
      ctx: Rule context.
      proto_info: ProtoInfo provider.

    Returns:
      A string specifying the output directory
    """
    proto_root = proto_info.proto_source_root
    if proto_root.startswith(ctx.bin_dir.path):
        path = proto_root
    else:
        path = ctx.bin_dir.path + "/" + proto_root

    if proto_root == ".":
        path = ctx.bin_dir.path
    return path

def proto_common_compile(ctx, proto_info, proto_lang_toolchain_info, generated_files):
    """A wrapper around proto_common.compile that automatically calculates the output dir.

    Args:
      ctx: Rule context.
      proto_info: ProtoInfo provider.
      proto_lang_toolchain_info: ProtoLangToolchainInfo provider.
      generated_files: The files we expect to be generated from this protoc invocation.
    """
    proto_common.compile(
        actions = ctx.actions,
        proto_info = proto_info,
        proto_lang_toolchain_info = proto_lang_toolchain_info,
        generated_files = generated_files,
        plugin_output = output_dir(ctx, proto_info),
    )
