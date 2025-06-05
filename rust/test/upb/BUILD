# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# Tests specific to upb kernel.
#
# Only add tests that are cpp kernel specific and it is not possible to make them work for upb (
# for example tests exercising ABI compatibility with other UPB-based Protobuf implementations).
#
# All the tests under this package should ignore
# `//rust:rust_proto_library_kernel` flag and should always select `upb`.
#
# To do that use:
# * `rust_upb_proto_library` instead of `rust_proto_library`.
# * `//rust:protobuf_upb_export` instead of
#   `//rust:protobuf`.

load("@rules_rust//rust:defs.bzl", "rust_test")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

# TODO: Enable this for the cpp kernel and move these tests to shared.
rust_test(
    name = "string_ctypes_test_upb_test",
    srcs = ["string_ctypes_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_proto3_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

# blaze test //rust/test/upb:debug_string_test --test_arg=--nocapture -c dbg
# --test_output=all to see debug string in test output logs.
rust_test(
    name = "debug_string_test",
    srcs = ["debug_string_test.rs"],
    deps = [
        "//rust:protobuf_upb",
        "//rust/test:map_unittest_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "thread_local_arena_test",
    srcs = ["thread_local_arena_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)
