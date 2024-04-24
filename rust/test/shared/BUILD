# Tests that are exercising and should pass with both kernels.
#
# All the tests under this package should ignore
# `//rust:rust_proto_library_kernel` flag and should always cover both
# kernels.
#
# To do that:
# * Declare 2 rust_test targets and share the same sources from both.
# * First copy will only depend on `rust_cc_proto_library` Rust proto targets, and if needed will
#   only depend on `//rust:protobuf_cpp_export`.
# * Second copy will only depend on `rust_upb_proto_library` Rust proto targets, and if needed will
#   only depend on `//rust:protobuf_upb_export`.
#
# Once we have a couple of these tests we will investigate ways to remove boilerplate (for example
# by introducing a build macro that registers 2 rust_test targets).

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_test")

rust_library(
    name = "matchers_upb",
    testonly = True,
    srcs = ["matchers.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [
        "//rust:protobuf_upb_export",
        "@crate_index//:googletest",
    ],
)

rust_library(
    name = "matchers_cpp",
    testonly = True,
    srcs = ["matchers.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    visibility = ["//rust/test/shared:__subpackages__"],
    deps = [
        "//rust:protobuf_cpp_export",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "child_parent_upb_test",
    srcs = ["child_parent_test.rs"],
    deps = [
        "//rust/test:child_upb_rust_proto",
        "//rust/test:parent_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "child_parent_cpp_test",
    srcs = ["child_parent_test.rs"],
    deps = [
        "//rust/test:child_cc_rust_proto",
        "//rust/test:parent_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "edition2023_cpp_test",
    srcs = ["edition2023_test.rs"],
    deps = [
        "//rust/test:edition2023_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "edition2023_upb_test",
    srcs = ["edition2023_test.rs"],
    deps = [
        "//rust/test:edition2023_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "enum_cpp_test",
    srcs = ["enum_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:enums_cc_rust_proto",
        "//rust/test:unittest_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "enum_upb_test",
    srcs = ["enum_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:enums_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "import_public_cpp_test",
    srcs = ["import_public_test.rs"],
    deps = [
        "//rust/test:import_public_cc_rust_proto",
    ],
)

rust_test(
    name = "import_public_upb_test",
    srcs = ["import_public_test.rs"],
    deps = [
        "//rust/test:import_public_upb_rust_proto",
    ],
)

rust_test(
    name = "package_cpp_test",
    srcs = ["package_test.rs"],
    deps = [
        "//rust/test:dots_in_package_cc_rust_proto",
        "//rust/test:no_package_cc_rust_proto",
        "//rust/test:package_cc_rust_proto",
    ],
)

rust_test(
    name = "package_upb_test",
    srcs = ["package_test.rs"],
    deps = [
        "//rust/test:dots_in_package_upb_rust_proto",
        "//rust/test:no_package_upb_rust_proto",
        "//rust/test:package_upb_rust_proto",
    ],
)

rust_test(
    name = "package_disambiguation_cpp_test",
    srcs = ["package_disambiguation_test.rs"],
    deps = ["//rust/test:package_disabiguation_cc_rust_proto"],
)

rust_test(
    name = "package_disambiguation_upb_test",
    srcs = ["package_disambiguation_test.rs"],
    deps = ["//rust/test:package_disabiguation_upb_rust_proto"],
)

rust_test(
    name = "reserved_cpp_test",
    srcs = ["reserved_test.rs"],
    deps = [
        "//rust/test:reserved_cc_rust_proto",
        "//rust/test:unittest_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "reserved_upb_test",
    srcs = ["reserved_test.rs"],
    deps = [
        "//rust/test:reserved_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "nested_types_cpp_test",
    srcs = ["nested_types_test.rs"],
    deps = ["//rust/test:unittest_cc_rust_proto"],
)

rust_test(
    name = "nested_types_upb_test",
    srcs = ["nested_types_test.rs"],
    deps = ["//rust/test:unittest_upb_rust_proto"],
)

rust_test(
    name = "accessors_cpp_test",
    srcs = ["accessors_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
        "//rust/test/shared:matchers_cpp": "matchers",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:unittest_cc_rust_proto",
        "//rust/test/shared:matchers_cpp",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_upb_test",
    srcs = ["accessors_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
        "//rust/test/shared:matchers_upb": "matchers",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_upb_rust_proto",
        "//rust/test/shared:matchers_upb",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_proto3_cpp_test",
    srcs = ["accessors_proto3_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
        "//rust/test/shared:matchers_cpp": "matchers",
    },
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:unittest_proto3_cc_rust_proto",
        "//rust/test:unittest_proto3_optional_cc_rust_proto",
        "//rust/test/shared:matchers_cpp",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_proto3_upb_test",
    srcs = ["accessors_proto3_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
        "//rust/test/shared:matchers_upb": "matchers",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_proto3_optional_upb_rust_proto",
        "//rust/test:unittest_proto3_upb_rust_proto",
        "//rust/test/shared:matchers_upb",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "serialization_upb_test",
    srcs = ["serialization_test.rs"],
    deps = [
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "serialization_cpp_test",
    srcs = ["serialization_test.rs"],
    deps = [
        "//rust/test:unittest_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "simple_nested_cpp_test",
    srcs = ["simple_nested_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:nested_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "simple_nested_upb_test",
    srcs = ["simple_nested_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:nested_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_repeated_cpp_test",
    srcs = ["accessors_repeated_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:unittest_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_repeated_upb_test",
    srcs = ["accessors_repeated_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_map_cpp_test",
    srcs = ["accessors_map_test.rs"],
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust/test:enums_cc_rust_proto",
        "//rust/test:map_unittest_cc_rust_proto",
        "//rust/test:unittest_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_map_upb_test",
    srcs = ["accessors_map_test.rs"],
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust/test:enums_upb_rust_proto",
        "//rust/test:map_unittest_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "reserved_crate_name_cc_test",
    srcs = ["reserved_crate_name_test.rs"],
    deps = [
        "//rust/test:struct_cc_rust_proto",
    ],
)

rust_test(
    name = "reserved_crate_name_upb_test",
    srcs = ["reserved_crate_name_test.rs"],
    deps = [
        "//rust/test:struct_upb_rust_proto",
    ],
)

rust_test(
    name = "fields_with_imported_types_cpp_test",
    srcs = ["fields_with_imported_types_test.rs"],
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:fields_with_imported_types_cc_rust_proto",
        "//rust/test:imported_types_cc_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "fields_with_imported_types_upb_test",
    srcs = ["fields_with_imported_types_test.rs"],
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:fields_with_imported_types_upb_rust_proto",
        "//rust/test:imported_types_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "proto_macro_cpp_test",
    srcs = ["proto_macro_test.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
        "//rust/test/shared:matchers_cpp": "matchers",
    },
    deps = [
        "//rust:protobuf_cpp",
        "//rust/test:unittest_cc_rust_proto",
        "//rust/test/shared:matchers_cpp",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "proto_macro_upb_test",
    srcs = ["proto_macro_test.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
        "//rust/test/shared:matchers_upb": "matchers",
    },
    deps = [
        "//rust:protobuf_upb",
        "//rust/test:unittest_upb_rust_proto",
        "//rust/test/shared:matchers_upb",
        "@crate_index//:googletest",
    ],
)
