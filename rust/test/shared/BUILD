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

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

rust_library(
    name = "matchers_upb",
    srcs = ["matchers.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
    },
    deps = [
        "@crate_index//:googletest",
        "//rust:protobuf_upb",
    ],
)

rust_library(
    name = "matchers_cpp",
    srcs = ["matchers.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
    },
    deps = [
        "@crate_index//:googletest",
        "//rust:protobuf_cpp",
    ],
)

rust_test(
    name = "child_parent_upb_test",
    srcs = ["child_parent_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:child_upb_rust_proto",
        "//rust/test:parent_upb_rust_proto",
    ],
)

rust_test(
    name = "child_parent_cpp_test",
    srcs = ["child_parent_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:child_cc_rust_proto",
        "//rust/test:parent_cc_rust_proto",
    ],
)

rust_test(
    name = "package_cpp_test",
    srcs = ["package_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "//rust/test:dots_in_package_cc_rust_proto",
        "//rust/test:no_package_cc_rust_proto",
        "//rust/test:package_cc_rust_proto",
    ],
)

rust_test(
    name = "package_upb_test",
    srcs = ["package_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "//rust/test:dots_in_package_upb_rust_proto",
        "//rust/test:no_package_upb_rust_proto",
        "//rust/test:package_upb_rust_proto",
    ],
)

rust_test(
    name = "reserved_cpp_test",
    srcs = ["reserved_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:reserved_cc_rust_proto",
        "//rust/test:unittest_cc_rust_proto",
    ],
)

rust_test(
    name = "reserved_upb_test",
    srcs = ["reserved_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:reserved_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
    ],
)

rust_test(
    name = "nested_messages_cpp_test",
    srcs = ["nested_messages_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_cc_rust_proto"],
)

rust_test(
    name = "nested_messages_upb_test",
    srcs = ["nested_messages_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = ["//rust/test:unittest_upb_rust_proto"],
)

rust_test(
    name = "accessors_cpp_test",
    srcs = ["accessors_test.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
        "//rust/test/shared:matchers_cpp": "matchers",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust:protobuf_cpp",
        "//rust/test:unittest_cc_rust_proto",
        "//rust/test/shared:matchers_cpp",
    ],
)

rust_test(
    name = "accessors_upb_test",
    srcs = ["accessors_test.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
        "//rust/test/shared:matchers_upb": "matchers",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust:protobuf_upb",
        "//rust/test:unittest_upb_rust_proto",
        "//rust/test/shared:matchers_upb",
    ],
)

rust_test(
    name = "accessors_proto3_cpp_test",
    srcs = ["accessors_proto3_test.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
        "//rust/test/shared:matchers_cpp": "matchers",
    },
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust:protobuf_cpp",
        "//rust/test:unittest_proto3_cc_rust_proto",
        "//rust/test:unittest_proto3_optional_cc_rust_proto",
        "//rust/test/shared:matchers_cpp",
    ],
)

rust_test(
    name = "accessors_proto3_upb_test",
    srcs = ["accessors_proto3_test.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
        "//rust/test/shared:matchers_upb": "matchers",
    },
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust:protobuf_upb",
        "//rust/test:unittest_proto3_optional_upb_rust_proto",
        "//rust/test:unittest_proto3_upb_rust_proto",
        "//rust/test/shared:matchers_upb",
    ],
)

rust_test(
    name = "serialization_upb_test",
    srcs = ["serialization_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:unittest_upb_rust_proto",
    ],
)

rust_test(
    name = "serialization_cpp_test",
    srcs = ["serialization_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:unittest_cc_rust_proto",
    ],
)

rust_test(
    name = "simple_nested_cpp_test",
    srcs = ["simple_nested_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:nested_cc_rust_proto",
    ],
)

rust_test(
    name = "simple_nested_upb_test",
    srcs = ["simple_nested_test.rs"],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:nested_upb_rust_proto",
    ],
)

rust_test(
    name = "accessors_repeated_cpp_test",
    srcs = ["accessors_repeated_test.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust:protobuf_cpp",
        "//rust/test:unittest_cc_rust_proto",
    ],
)

rust_test(
    name = "accessors_repeated_upb_test",
    srcs = ["accessors_repeated_test.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust:protobuf_upb",
        "//rust/test:unittest_upb_rust_proto",
    ],
)

rust_test(
    name = "accessors_map_cpp_test",
    srcs = ["accessors_map_test.rs"],
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:map_unittest_cc_rust_proto",
    ],
)

rust_test(
    name = "accessors_map_upb_test",
    srcs = ["accessors_map_test.rs"],
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    tags = [
        # TODO: Enable testing on arm once we support sanitizers for Rust on Arm.
        "not_build:arm",
    ],
    deps = [
        "@crate_index//:googletest",
        "//rust/test:map_unittest_upb_rust_proto",
    ],
)
