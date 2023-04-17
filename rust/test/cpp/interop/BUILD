# This package contains tests exercising C++/Rust interop in the cpp kernel.

load("@rules_rust//rust:defs.bzl", "rust_test")

cc_library(
    name = "test_utils",
    srcs = ["test_utils.cc"],
    deps = [
        "//rust/cpp_kernel:cpp_api",
        "//rust/test:unittest_cc_proto",
    ],
)

rust_test(
    name = "interop_test",
    srcs = ["main.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = [
        ":test_utils",
        "//rust:protobuf_cpp",
        "//rust/test:unittest_cc_rust_proto",
    ],
)
