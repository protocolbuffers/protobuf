# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_rust//rust:defs.bzl", "rust_library")

licenses(["notice"])  # MIT or Apache 2.0

rust_library(
    name = "linkme",
    srcs = glob(["src/**/*.rs"]),
    crate_root = "src/lib.rs",
    edition = "2021",
    proc_macro_deps = [
        "@crates_io_linkme_impl//:linkme_impl",
    ],
    version = "0.3.36",
    visibility = ["//visibility:public"],
)
