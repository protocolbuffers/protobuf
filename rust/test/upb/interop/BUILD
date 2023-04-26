# This package contains tests exercising UPB/Rust interop in the UPB kernel.

load("@rules_rust//rust:defs.bzl", "rust_test")

cc_library(
    name = "test_utils",
    testonly = True,
    srcs = ["test_utils.cc"],
    visibility = ["//rust/test:__subpackages__"],
    deps = [
        "//rust/test:unittest_upb_proto",
        "@upb//:upb",
    ],
)
