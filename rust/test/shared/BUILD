# Tests that are exercising and should pass with both kernels.
#
# All the tests under this package should ignore
# `//rust:rust_proto_library_kernel` flag and should always cover both
# kernels.
#
# To do that:
# * Declare 2 rust_test targets and share the same sources from both.
# * First copy will only depend on `rust_cc_proto_library` Rust proto targets, and if needed will
#   only depend on `//rust:protobuf_cpp.
# * Second copy will only depend on `rust_upb_proto_library` Rust proto targets, and if needed will
#   only depend on `//rust:protobuf_upb
#
# Once we have a couple of these tests we will investigate ways to remove boilerplate (for example
# by introducing a build macro that registers 2 rust_test targets).

load("@rules_rust//rust:defs.bzl", "rust_test")

rust_test(
    name = "unittest_proto_upb_test",
    srcs = ["unittest_proto_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
        # TODO(b/243126140): Enable tsan once we support sanitizers with Rust.
        "notsan",
        # TODO(b/243126140): Enable msan once we support sanitizers with Rust.
        "nomsan",
    ],
    deps = ["//rust/test:unittest_upb_rust_proto"],
)

rust_test(
    name = "unittest_proto_cpp_test",
    srcs = ["unittest_proto_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
        # TODO(b/243126140): Enable tsan once we support sanitizers with Rust.
        "notsan",
        # TODO(b/243126140): Enable msan once we support sanitizers with Rust.
        "nomsan",
    ],
    deps = ["//rust/test:unittest_cc_rust_proto"],
)

rust_test(
    name = "child_parent_upb_test",
    srcs = ["child_parent_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
        # TODO(b/243126140): Enable tsan once we support sanitizers with Rust.
        "notsan",
        # TODO(b/243126140): Enable msan once we support sanitizers with Rust.
        "nomsan",
    ],
    deps = [
        "//rust/test:child_upb_rust_proto",
        "//rust/test:parent_upb_rust_proto",
    ],
)

rust_test(
    name = "child_parent_cpp_test",
    srcs = ["child_parent_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = [
        "//rust/test:child_cc_rust_proto",
        "//rust/test:parent_cc_rust_proto",
    ],
)

rust_test(
    name = "package_cpp_test",
    srcs = ["package_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = [
        "//rust/test:dots_in_package_cc_rust_proto",
        "//rust/test:no_package_cc_rust_proto",
        "//rust/test:unittest_cc_rust_proto",
    ],
)

rust_test(
    name = "package_upb_test",
    srcs = ["package_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = [
        "//rust/test:dots_in_package_upb_rust_proto",
        "//rust/test:no_package_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
    ],
)

rust_test(
    name = "nested_messages_cpp_test",
    srcs = ["nested_messages_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_cc_rust_proto"],
)

rust_test(
    name = "nested_messages_upb_test",
    srcs = ["nested_messages_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_upb_rust_proto"],
)

rust_test(
    name = "accessors_cpp_test",
    srcs = ["accessors_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_cc_rust_proto"],
)

rust_test(
    name = "accessors_upb_test",
    srcs = ["accessors_test.rs"],
    tags = [
        # TODO(b/270274576): Enable testing on arm once we have a Rust Arm toolchain.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_upb_rust_proto"],
)
