# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_shell//shell:sh_test.bzl", "sh_test")

package(default_applicable_licenses = ["//:license"])

config_setting(
    name = "android",
    constraint_values = ["@platforms//os:android"],
)

config_setting(
    name = "wasm",
    constraint_values = ["@platforms//os:emscripten"],
)

config_setting(
    name = "ios",
    constraint_values = ["@platforms//os:ios"],
)

PLATFORM_ARGS = select({
    ":android": ["android"],
    ":wasm": ["wasm"],
    ":ios": ["ios"],
    "//conditions:default": ["not_supported"],
})

_NM_TOOL_TARGET = []

_NM_TOOL_PATH = ["nm"]

_OBJDUMP_TOOL_TARGET = []

_OBJDUMP_TOOL_PATH = ["objdump"]

_UNUSED_SYMBOLS_APP_TARGET = ["//rust/test/treeshaking/base:treeshaking_app"]

_UNUSED_SYMBOLS_APP_PATH = ["$(rootpath //rust/test/treeshaking/base:treeshaking_app)"]

_BASE_APP_TARGET = ["//rust/test/treeshaking/base:treeshaking_app"]

_BASE_APP_PATH = ["$(rootpath //rust/test/treeshaking/base:treeshaking_app)"]

_TEST_APP_TARGET = ["//rust/test/treeshaking/test:treeshaking_app"]

_TEST_APP_PATH = ["$(rootpath //rust/test/treeshaking/test:treeshaking_app)"]

# This works on all platforms today because unused symbols are correctly stripped.
sh_test(
    name = "unused_symbols_test",
    srcs = ["verify_unused_symbols.sh"],
    args = _UNUSED_SYMBOLS_APP_PATH + _NM_TOOL_PATH + PLATFORM_ARGS,
    data = _UNUSED_SYMBOLS_APP_TARGET + _NM_TOOL_TARGET,
)

# This only works on platforms with panic=abort or panic=immediate-abort.
sh_test(
    name = "except_tables_test",
    srcs = ["verify_except_tables.sh"],
    args = _BASE_APP_PATH + _TEST_APP_PATH + _NM_TOOL_PATH + PLATFORM_ARGS + _OBJDUMP_TOOL_PATH,
    data = _BASE_APP_TARGET + _TEST_APP_TARGET + _NM_TOOL_TARGET + _OBJDUMP_TOOL_TARGET,
)
