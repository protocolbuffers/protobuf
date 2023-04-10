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
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
        # TODO(b/243126140): Enable tsan once we support sanitizers with Rust.
        "notsan",
        # TODO(b/243126140): Enable msan once we support sanitizers with Rust.
        "nomsan",
    ],
)
