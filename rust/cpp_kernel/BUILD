# This package contains Rust protobuf runtime implementation built on top of the C++ backend.

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

# Represents Rust Protobuf runtime using the cpp kernel.
#
# `rust_cpp_proto_library` implicitly depends on this target. This target cannot depend on
# `:rust_proto_library_kernel` build setting; it has to be fully functional under any value of that
# setting.
rust_library(
    name = "cpp",
    srcs = ["cpp.rs"],
    visibility = [
        "//src/google/protobuf:__subpackages__",
        "//rust:__subpackages__",
    ],
    deps = ["//rust:common"],
)

rust_test(
    name = "cpp_test",
    crate = ":cpp",
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
)

cc_library(
    name = "cpp_api",
    hdrs = ["cpp_api.h"],
    visibility = [
        "//src/google/protobuf:__subpackages__",
        "//rust:__subpackages__",
    ],
    deps = [
        ":rust_alloc_for_cpp_api",
        "//src/google/protobuf",
    ],
)

rust_library(
    name = "rust_alloc_for_cpp_api",
    srcs = ["rust_alloc_for_cpp_api.rs"],
)
