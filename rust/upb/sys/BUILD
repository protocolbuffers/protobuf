# Copyright 2024 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files", "strip_prefix")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

package(default_applicable_licenses = ["//:license"])

rust_library(
    name = "upb_sys",
    srcs = [
        "arena.rs",
        "array.rs",
        "ctype.rs",
        "extension_registry.rs",
        "map.rs",
        "message.rs",
        "message_value.rs",
        "mini_table.rs",
        "mod.rs",
        "opaque_pointee.rs",
        "string_view.rs",
        "text.rs",
        "wire.rs",
    ],
    crate_root = "mod.rs",
    rustc_flags = ["--cfg=bzl"],
    visibility = [
        "//rust:__subpackages__",
        "//rust/upb:__subpackages__",
    ],
    deps = [":upb_c_api"],
)

rust_test(
    name = "sys_rs_crate_test",
    crate = ":sys",
)

cc_library(
    name = "upb_c_api",
    srcs = ["upb_api.c"],
    deps = [
        "//upb:mem",
        "//upb:message",
        "//upb:message_compare",
        "//upb:message_copy",
        "//upb/mini_table",
        "//upb/text:debug",
    ],
)
