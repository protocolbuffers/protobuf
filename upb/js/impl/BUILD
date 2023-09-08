# Copyright 2023 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Implementation of the JavaScript layer for upb.js.

load("//javascript/typescript:build_defs.bzl", "ts_library")

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

ts_library(
    name = "impl",
    srcs = [
        "accessor_registry.ts",
        "arena.ts",
        "buffer.ts",
        "enum_accessor.ts",
        "extension_accessor.ts",
        "internal_message.ts",
        "message_accessor_nobits.ts",
        "message_accessor_withbits.ts",
        "mini_table.ts",
        "strings.ts",
        "upb_bits.ts",
        "upb_impl.ts",
        "wasm_exports.ts",
        "wasm_imports.ts",
    ],
    data = ["@upb//js:upb_js_wasm"],
    visibility = ["@upb//js:__pkg__"],
    deps = [
        ":wasm_mem",
        "//third_party/javascript/node_modules/typescript:es2021.weakref",
        "//third_party/javascript/node_modules/typescript:esnext.bigint",
        "@upb//bits/typescript",
        "@upb//js:upb_externs",
    ],
)

ts_library(
    name = "wasm_mem",
    srcs = ["wasm_mem.ts"],
    visibility = ["@upb//bits/typescript:__pkg__"],
    deps = [
        "//javascript/common/asserts",
        "//third_party/javascript/node_modules/typescript:esnext.bigint",
    ],
)
