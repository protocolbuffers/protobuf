# Tests specific to cpp kernel.
#
# Only add tests that are cpp kernel specific and it is not possible to make them work for upb (
# for example tests exercising ABI compatibility with C++ Protobuf API).
#
# All the tests under this package should ignore
# `//rust:rust_proto_library_kernel` flag and should always select `cpp`.
#
# To do that use:
# * `rust_cc_proto_library` instead of `rust_proto_library`.
# * `//rust:protobuf_cpp` instead of `//rust:protobuf``.

load("@rules_rust//rust:defs.bzl", "rust_test")

rust_test(
    name = "accessors_test",
    srcs = ["accessors_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_cc_rust_proto"],
)

rust_test(
    name = "nested_messages_test",
    srcs = ["nested_messages_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_cc_rust_proto"],
)
