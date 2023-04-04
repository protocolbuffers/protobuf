# This package contains Rust protobuf runtime implementation built on top of UPB.

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

rust_library(
    name = "upb",
    srcs = ["upb.rs"],
    visibility = [
        "//src/google/protobuf:__subpackages__",
        "//rust:__subpackages__",
    ],
    deps = [":upb_c_api"],
)

rust_test(
    name = "upb_test",
    crate = ":upb",
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
        # TODO(b/243126140): Enable tsan once we support sanitizers with Rust.
        "notsan",
        # TODO(b/243126140): Enable msan once we support sanitizers with Rust.
        "nomsan",
    ],
)

cc_library(
    name = "upb_c_api",
    srcs = ["upb_api.c"],
    deps = [
        "@upb//:upb",
    ],
)
