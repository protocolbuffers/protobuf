# Copyright 2024 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_pkg//pkg:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

package(default_applicable_licenses = ["//:license"])

rust_library(
    name = "upb",
    srcs = [
        "arena.rs",
        "associated_mini_table.rs",
        "lib.rs",
        "message.rs",
        "mini_table.rs",
        "owned_arena_box.rs",
        "text.rs",
        "wire.rs",
    ],
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    rustc_flags = ["--cfg=bzl"],
    visibility = [
        "//rust:__subpackages__",
        "//src/google/protobuf:__subpackages__",
    ],
    deps = [
        "//rust/upb/sys",
    ],
)

rust_test(
    name = "upb_rs_crate_test",
    crate = ":upb",
    rustc_flags = ["--cfg=bzl"],
    deps = [
        "@crate_index//:googletest",
    ],
)

pkg_files(
    name = "src",
    srcs = glob(["*"]) + [
        "//rust/upb/sys:srcs",
    ],
    prefix = "upb",
    strip_prefix = strip_prefix.from_root("rust/upb"),
    visibility = [
        "//rust:__subpackages__",
        "//src/google/protobuf:__subpackages__",
    ],
)
