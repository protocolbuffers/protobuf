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

"""Macro wrapping the proto_toolchain implementation.

The macro additionally creates toolchain target when toolchain_type is given.
"""

load("//src/google/protobuf/github/proto/private:proto_toolchain_rule.bzl", _proto_toolchain_rule = "proto_toolchain")

def proto_toolchain(*, name, proto_compiler, exec_compatible_with = []):
    """Creates a proto_toolchain and toolchain target for proto_library.

    Toolchain target is suffixed with "_toolchain".

    Args:
      name: name of the toolchain
      proto_compiler: (Label) of either proto compiler sources or prebuild binaries
      exec_compatible_with: ([constraints]) List of constraints the prebuild binary is compatible with.
    """
    _proto_toolchain_rule(name = name, proto_compiler = proto_compiler)

    native.toolchain(
        name = name + "_toolchain",
        toolchain_type = "//third_party/bazel_rules/rules_proto/proto:toolchain_type",
        exec_compatible_with = exec_compatible_with,
        target_compatible_with = [],
        toolchain = name,
    )
