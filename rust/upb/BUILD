# Copyright 2024 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_pkg//pkg:mappings.bzl", "pkg_filegroup", "pkg_files", "strip_prefix")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

package(default_applicable_licenses = ["//:license"])

rust_library(
    name = "upb",
    srcs = [
        "arena.rs",
        "associated_mini_table.rs",
        "lib.rs",
        "owned_arena_box.rs",
        "text.rs",
        "wire.rs",
    ],
    rustc_flags = ["--cfg=bzl"],
    visibility = [
        "//rust:__subpackages__",
        "//src/google/protobuf:__subpackages__",
    ],
    deps = ["//third_party/upb/rust/sys:upb_sys"],
)

rust_test(
    name = "upb_rs_crate_test",
    crate = ":upb",
)

pkg_files(
    name = "rust_protobuf_upb_src",
    srcs = glob(
        [
            "*",
        ],
    ),
    strip_prefix = strip_prefix.from_root("rust"),
    visibility = ["//rust:__pkg__"],
)
