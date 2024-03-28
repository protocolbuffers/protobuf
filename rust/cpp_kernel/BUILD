# This package contains Rust protobuf runtime implementation built on top of the C++ backend.

load("@rules_rust//rust:defs.bzl", "rust_library")

cc_library(
    name = "cpp_api",
    srcs = ["cpp_api.cc"],
    hdrs = ["cpp_api.h"],
    visibility = [
        "//rust:__subpackages__",
        "//src/google/protobuf:__subpackages__",
    ],
    deps = [
        ":rust_alloc_for_cpp_api",  # buildcleaner: keep
        "//src/google/protobuf",
        "//src/google/protobuf:protobuf_lite",
        "@com_google_absl//absl/strings:string_view",
    ],
)

rust_library(
    name = "rust_alloc_for_cpp_api",
    srcs = ["rust_alloc_for_cpp_api.rs"],
    visibility = [
        "//rust:__subpackages__",
    ],
)
