# Copyright 2023 The Bazel Authors. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""A Starlark implementation of the proto_toolchain rule."""

load("//src/google/protobuf/github/proto/common:proto_common.bzl", "proto_common")
load("//src/google/protobuf/github/proto/common:proto_lang_toolchain_info.bzl", "ProtoLangToolchainInfo")

def _impl(ctx):
    kwargs = {}
    if getattr(proto_common, "INCOMPATIBLE_PASS_TOOLCHAIN_TYPE", False):
        kwargs["toolchain_type"] = "//third_party/bazel_rules/rules_proto/proto:toolchain_type"

    return [
        DefaultInfo(
            files = depset(),
            runfiles = ctx.runfiles(),
        ),
        platform_common.ToolchainInfo(
            proto = ProtoLangToolchainInfo(
                out_replacement_format_flag = ctx.attr.command_line,
                output_files = ctx.attr.output_files,
                plugin = None,
                runtime = None,
                proto_compiler = ctx.attr.proto_compiler.files_to_run,
                protoc_opts = ctx.fragments.proto.experimental_protoc_opts,
                progress_message = ctx.attr.progress_message,
                mnemonic = ctx.attr.mnemonic,
                **kwargs
            ),
        ),
    ]

proto_toolchain = rule(
    _impl,
    attrs =
        {
            "progress_message": attr.string(default = "Generating Descriptor Set proto_library %{label}"),
            "mnemonic": attr.string(default = "GenProtoDescriptorSet"),
            "command_line": attr.string(default = "--descriptor_set_out=%s"),
            "output_files": attr.string(values = ["single", "multiple", "legacy"], default = "single"),
            "proto_compiler": attr.label(
                cfg = "exec",
                executable = True,
                allow_files = True,  # Used by mocks in tests. Consider fixing tests and removing it.
            ),
        },
    provides = [platform_common.ToolchainInfo],
    fragments = ["proto"],
)
