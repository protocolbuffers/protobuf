# This package contains tests exercising C++/Rust interop in the cpp kernel.

load("@rules_rust//rust:defs.bzl", "rust_test")

cc_library(
    name = "test_utils",
    srcs = ["test_utils.cc"],
    deps = [
        "//rust/cpp_kernel:cpp_api",
        "//src/google/protobuf:unittest_cc_proto",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/strings",
    ],
)

rust_test(
    name = "interop_test",
    srcs = ["main.rs"],
    deps = [
        ":test_utils",
        "//rust:protobuf_cpp",
        "//src/google/protobuf:unittest_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)
