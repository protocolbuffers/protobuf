# Tests specific to cpp kernel.
#
# Only add tests that are cpp kernel specific and it is not possible to make them work for upb (
# for example tests exercising ABI compatibility with C++ Protobuf API).
#
# All the tests under this package should ignore
# `//rust:rust_proto_library_kernel` flag and should always select `cpp`.
#
# To do that use:
# * `rust_cc_proto_library` instead of `rust_proto_library`.
# * `//rust:protobuf_cpp_export` instead of
#   `//rust:protobuf`.

load("@rules_rust//rust:defs.bzl", "rust_test")
load("//bazel:proto_library.bzl", "proto_library")
load(
    "//rust:defs.bzl",
    "rust_cc_proto_library",
)

proto_library(
    name = "debug_proto",
    testonly = True,
    srcs = ["debug.proto"],
)

rust_cc_proto_library(
    name = "debug_cpp_rust_proto",
    testonly = True,
    deps = [":debug_proto"],
)

proto_library(
    name = "optimize_for_lite_proto",
    testonly = True,
    srcs = ["optimize_for_lite.proto"],
)

rust_cc_proto_library(
    name = "optimize_for_lite_cpp_rust_proto",
    testonly = True,
    visibility = ["//visibility:private"],
    deps = [":optimize_for_lite_proto"],
)

rust_test(
    name = "debug_test",
    srcs = ["debug_test.rs"],
    deps = [
        ":debug_cpp_rust_proto",
        ":optimize_for_lite_cpp_rust_proto",
        "//rust:protobuf_cpp_export",
        "@crate_index//:googletest",
    ],
)
