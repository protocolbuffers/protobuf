# Copyright 2019 The Bazel Authors. All rights reserved.
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
"""proto_lang_toolchain rule"""

load("//src/google/protobuf/github/proto/common:proto_common.bzl", "proto_common")

def proto_lang_toolchain(*, name, toolchain_type = None, exec_compatible_with = [], target_compatible_with = [], **attrs):
    """Creates a proto_lang_toolchain and corresponding toolchain target.

    Toolchain target is only created when toolchain_type is set.

    https://docs.bazel.build/versions/master/be/protocol-buffer.html#proto_lang_toolchain

    Args:

      name: name of the toolchain
      toolchain_type: The toolchain type
      exec_compatible_with: ([constraints]) List of constraints the prebuild binaries is compatible with.
      target_compatible_with: ([constraints]) List of constraints the target libraries are compatible with.
      **attrs: Rule attributes
    """

    if getattr(proto_common, "INCOMPATIBLE_PASS_TOOLCHAIN_TYPE", False):
        attrs["toolchain_type"] = toolchain_type

    # buildifier: disable=native-proto
    native.proto_lang_toolchain(name = name, **attrs)

    if toolchain_type:
        native.toolchain(
            name = name + "_toolchain",
            toolchain_type = toolchain_type,
            exec_compatible_with = exec_compatible_with,
            target_compatible_with = target_compatible_with,
            toolchain = name,
        )
