# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_shell//shell:sh_test.bzl", "sh_test")

package(default_applicable_licenses = ["//:license", "//:package_metadata"])

config_setting(
    name = "android",
    constraint_values = ["@platforms//os:android"],
)

config_setting(
    name = "wasm",
    constraint_values = ["@platforms//os:emscripten"],
)

PLATFORM_ARGS = select({
    ":android": ["android"],
    ":wasm": ["wasm"],
    "//conditions:default": ["not_supported"],
})

_NM_TOOL_TARGET = []

_NM_TOOL_PATH = ["nm"]

# This works on all platforms today because unused symbols are correctly stripped.
sh_test(
    name = "unused_symbols_test",
    srcs = ["verify_unused_symbols.sh"],
    args = [
        "$(rootpath //rust/test/treeshaking/base:treeshaking_app)",
    ] + _NM_TOOL_PATH + PLATFORM_ARGS,
    data = [
        "//rust/test/treeshaking/base:treeshaking_app",
    ] + _NM_TOOL_TARGET,
)

# This only works on platforms with panic=abort or panic=immediate-abort.
sh_test(
    name = "except_tables_test",
    srcs = ["verify_except_tables.sh"],
    args = [
        "$(rootpath //rust/test/treeshaking/base:treeshaking_app)",
        "$(rootpath //rust/test/treeshaking/test:treeshaking_app)",
    ] + _NM_TOOL_PATH + PLATFORM_ARGS,
    data = [
        "//rust/test/treeshaking/base:treeshaking_app",
        "//rust/test/treeshaking/test:treeshaking_app",
    ] + _NM_TOOL_TARGET,
)
