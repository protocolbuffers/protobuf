# Copyright 2025 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

package(default_applicable_licenses = ["@com_google_protobuf//:license"])

rust_library(
    name = "sys",
    srcs = [
        "base/ctype.rs",
        "base/mod.rs",
        "base/string_view.rs",
        "lib.rs",
        "mem/arena.rs",
        "mem/mod.rs",
        "message/array.rs",
        "message/map.rs",
        "message/message.rs",
        "message/message_value.rs",
        "message/mod.rs",
        "mini_table/extension_registry.rs",
        "mini_table/mini_table.rs",
        "mini_table/mod.rs",
        "opaque_pointee.rs",
        "test_helpers.rs",
        "text/mod.rs",
        "text/text.rs",
        "wire/mod.rs",
        "wire/wire.rs",
    ],
    rustc_flags = ["--cfg=bzl"],
    visibility = ["//upb:__subpackages__"],
    deps = [":upb_c_api"],
)

rust_test(
    name = "sys_test",
    crate = ":sys",
    deps = [
        "@crate_index//:googletest",
    ],
)

cc_library(
    name = "upb_c_api",
    srcs = ["upb_api.c"],
    deps = [
        "@com_google_protobuf//upb/mem",
        "@com_google_protobuf//upb/message",
        "@com_google_protobuf//upb/message:compare",
        "@com_google_protobuf//upb/message:copy",
        "@com_google_protobuf//upb/mini_descriptor",
        "@com_google_protobuf//upb/mini_table",
        "@com_google_protobuf//upb/text:debug",
    ],
)

pkg_files(
    name = "srcs",
    srcs = glob(["**/*"]),
    strip_prefix = strip_prefix.from_root("upb"),
    visibility = [
        "//upb:__subpackages__",
    ],
)
