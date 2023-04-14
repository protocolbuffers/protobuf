# Tests specific to upb kernel.
#
# Only add tests that are cpp kernel specific and it is not possible to make them work for upb (
# for example tests exercising ABI compatibility with other UPB-based Protobuf implementations).
#
# All the tests under this package should ignore
# `//rust:rust_proto_library_kernel` flag and should always select `upb`.
#
# To do that use:
# * `rust_upb_proto_library` instead of `rust_proto_library`.
# * `//rust:protobuf_upb` instead of `//rust:protobuf``.

load("@rules_rust//rust:defs.bzl", "rust_test")

rust_test(
    name = "accessors_test",
    srcs = ["accessors_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_upb_rust_proto"],
)
