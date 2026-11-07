# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_rust//rust:defs.bzl", "rust_library")

licenses(["notice"])  # MIT or Apache 2.0

rust_library(
    name = "syn",
    srcs = glob(["src/**/*.rs"]),
    crate_features = [
        "clone-impls",
        "default",
        "derive",
        "full",
        "parsing",
        "printing",
        "proc-macro",
    ],
    crate_root = "src/lib.rs",
    edition = "2021",
    version = "2.0.119",
    visibility = ["//visibility:public"],
    deps = [
        "@crates_io_proc_macro2//:proc_macro2",
        "@crates_io_quote//:quote",
        "@crates_io_unicode_ident//:unicode_ident",
    ],
)
