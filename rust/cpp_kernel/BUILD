# This package contains Rust protobuf runtime implementation built on top of the C++ backend.

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

rust_library(
    name = "cpp",
    srcs = ["cpp.rs"],
    visibility = [
        "//src/google/protobuf:__subpackages__",
        "//rust:__subpackages__",
    ],
)

rust_test(
    name = "cpp_test",
    crate = ":cpp",
    tags = [
        "not_build:arm",
        "notsan",
    ],
)
