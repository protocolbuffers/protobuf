# This package contains Rust protobuf runtime implementation built on top of UPB.

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

rust_library(
    name = "upb",
    srcs = ["upb.rs"],
    visibility = ["//src/google/protobuf:__subpackages__"],
    deps = [":upb_c_api"],
)

rust_test(
    name = "upb_test",
    crate = ":upb",
    tags = [
        "not_build:arm",
        "notsan",
    ],
)

cc_library(
    name = "upb_c_api",
    srcs = ["upb_api.c"],
    deps = [
        "@upb//:upb",
    ],
)
