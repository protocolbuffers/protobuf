# This package contains tests exercising C++/Rust interop in the cpp kernel.

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_rust//rust:defs.bzl", "rust_test")
load("@com_google_protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@com_google_protobuf//bazel:proto_library.bzl", "proto_library")
load(
    "//:defs.bzl",
    "rust_cc_proto_library",
)

cc_library(
    name = "test_utils",
    srcs = ["test_utils.cc"],
    deps = [
        ":interop_test_cc_proto",
        "//cpp_kernel:cpp_api",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
    ],
)

rust_test(
    name = "interop_test",
    srcs = ["main.rs"],
    deps = [
        ":interop_test_cpp_rust_proto",
        ":test_utils",
        "//:protobuf_cpp",
        "@crate_index//:googletest",
    ],
)

proto_library(
    name = "interop_test_proto",
    srcs = ["interop_test.proto"],
)

cc_proto_library(
    name = "interop_test_cc_proto",
    deps = [":interop_test_proto"],
)

rust_cc_proto_library(
    name = "interop_test_cpp_rust_proto",
    deps = [":interop_test_proto"],
)
