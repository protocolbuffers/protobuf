# This package contains Rust protobuf runtime implementation built on top of UPB.

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

# Represents Rust Protobuf runtime using the upb kernel.
#
# `rust_upb_proto_library` implicitly depends on this target. This target cannot depend on
# `:rust_proto_library_kernel` build setting; it has to be fully functional under any value of that
# setting.
rust_library(
    name = "upb",
    srcs = ["upb.rs"],
    visibility = [
        "//src/google/protobuf:__subpackages__",
        "//rust:__subpackages__",
    ],
    deps = [
        ":upb_c_api",
        "//rust:common",
    ],
)

rust_test(
    name = "upb_test",
    crate = ":upb",
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
)

cc_library(
    name = "upb_c_api",
    srcs = ["upb_api.c"],
    deps = [
        "@upb//:upb",
    ],
)
