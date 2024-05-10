# Copyright 2024 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

rust_library(
    name = "upb",
    srcs = [
        "arena.rs",
        "array.rs",
        "ctype.rs",
        "extension_registry.rs",
        "lib.rs",
        "map.rs",
        "message.rs",
        "message_value.rs",
        "mini_table.rs",
        "opaque_pointee.rs",
        "owned_arena_box.rs",
        "string_view.rs",
        "wire.rs",
    ],
    visibility = [
        "//rust:__subpackages__",
        "//src/google/protobuf:__subpackages__",
    ],
    deps = [":upb_c_api"],
)

rust_test(
    name = "upb_rs_crate_test",
    crate = ":upb",
)

cc_library(
    name = "upb_c_api",
    srcs = ["upb_api.c"],
    deps = [
        "//upb:mem",
        "//upb:message",
        "//upb:message_copy",
        "//upb/mini_table",
    ],
)
