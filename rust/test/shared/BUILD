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

load("@rules_rust//rust:defs.bzl", "rust_test")

rust_test(
    name = "ctype_cord_upb_test",
    srcs = ["ctype_cord_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "ctype_cord_cpp_test",
    srcs = ["ctype_cord_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:unittest_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "child_parent_upb_test",
    srcs = ["child_parent_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:child_upb_rust_proto",
        "//rust/test:parent_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "child_parent_cpp_test",
    srcs = ["child_parent_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:child_cpp_rust_proto",
        "//rust/test:parent_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "edition2023_cpp_test",
    srcs = ["edition2023_test.rs"],
    deps = [
        "//rust/test:edition2023_cpp_rust_proto",
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
        "//rust/test:enums_cpp_rust_proto",
        "//rust/test:unittest_cpp_rust_proto",
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
        "//rust/test:import_public_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "import_public_upb_test",
    srcs = ["import_public_test.rs"],
    deps = [
        "//rust/test:import_public_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "package_cpp_test",
    srcs = ["package_test.rs"],
    deps = [
        "//rust/test:dots_in_package_cpp_rust_proto",
        "//rust/test:no_package_cpp_rust_proto",
        "//rust/test:package_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "package_upb_test",
    srcs = ["package_test.rs"],
    deps = [
        "//rust/test:dots_in_package_upb_rust_proto",
        "//rust/test:no_package_upb_rust_proto",
        "//rust/test:package_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "package_disambiguation_cpp_test",
    srcs = ["package_disambiguation_test.rs"],
    deps = ["//rust/test:package_disabiguation_cpp_rust_proto"],
)

rust_test(
    name = "package_disambiguation_upb_test",
    srcs = ["package_disambiguation_test.rs"],
    deps = ["//rust/test:package_disabiguation_upb_rust_proto"],
)

rust_test(
    name = "bad_names_cpp_test",
    srcs = ["bad_names_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:bad_names_cpp_rust_proto",
        "//rust/test:unittest_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "bad_names_upb_test",
    srcs = ["bad_names_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:bad_names_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "nested_types_cpp_test",
    srcs = ["nested_types_test.rs"],
    deps = [
        "//rust/test:unittest_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "nested_types_upb_test",
    srcs = ["nested_types_test.rs"],
    deps = [
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_cpp_test",
    srcs = ["accessors_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:unittest_cpp_rust_proto",
        "//rust/test:unittest_import_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_upb_test",
    srcs = ["accessors_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_import_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_proto3_cpp_test",
    srcs = ["accessors_proto3_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:unittest_proto3_cpp_rust_proto",
        "//rust/test:unittest_proto3_optional_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_proto3_upb_test",
    srcs = ["accessors_proto3_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_proto3_optional_upb_rust_proto",
        "//rust/test:unittest_proto3_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "serialization_upb_test",
    srcs = ["serialization_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:unittest_proto3_optional_upb_rust_proto",
        "//rust/test:unittest_proto3_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "serialization_cpp_test",
    srcs = ["serialization_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:unittest_cpp_rust_proto",
        "//rust/test:unittest_proto3_cpp_rust_proto",
        "//rust/test:unittest_proto3_optional_cpp_rust_proto",
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
        "//rust/test:nested_cpp_rust_proto",
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
        "//rust/test:unittest_cpp_rust_proto",
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
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:enums_cpp_rust_proto",
        "//rust/test:map_unittest_cpp_rust_proto",
        "//rust/test:unittest_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "accessors_map_upb_test",
    srcs = ["accessors_map_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_upb_export",
        "//rust/test:enums_upb_rust_proto",
        "//rust/test:map_unittest_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "fields_with_imported_types_cpp_test",
    srcs = ["fields_with_imported_types_test.rs"],
    deps = [
        "//rust:protobuf_cpp_export",
        "//rust/test:fields_with_imported_types_cpp_rust_proto",
        "//rust/test:imported_types_cpp_rust_proto",
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
    name = "merge_from_cpp_test",
    srcs = ["merge_from_test.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp",
        "//rust/test:unittest_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "merge_from_upb_test",
    srcs = ["merge_from_test.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "proto_macro_cpp_test",
    srcs = ["proto_macro_test.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp",
        "//rust/test:map_unittest_cpp_rust_proto",
        "//rust/test:unittest_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "proto_macro_upb_test",
    srcs = ["proto_macro_test.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
        "//rust:protobuf_gtest_matchers_upb": "protobuf_gtest_matchers",
    },
    deps = [
        "//rust:protobuf_gtest_matchers_upb",
        "//rust:protobuf_upb",
        "//rust/test:map_unittest_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "gtest_matchers_cpp_test",
    srcs = ["gtest_matchers_test.rs"],
    aliases = {
        "//rust:protobuf_cpp": "protobuf",
        "//rust:protobuf_gtest_matchers_cpp": "protobuf_gtest_matchers",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_cpp",
        "//rust:protobuf_gtest_matchers_cpp",
        "//rust/test:unittest_cpp_rust_proto",
        "//rust/test:unittest_proto3_cpp_rust_proto",
        "//rust/test:unittest_proto3_optional_cpp_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "gtest_matchers_upb_test",
    srcs = ["gtest_matchers_test.rs"],
    aliases = {
        "//rust:protobuf_upb": "protobuf",
        "//rust:protobuf_gtest_matchers_upb": "protobuf_gtest_matchers",
    },
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    deps = [
        "//rust:protobuf_gtest_matchers_upb",
        "//rust:protobuf_upb",
        "//rust/test:unittest_proto3_optional_upb_rust_proto",
        "//rust/test:unittest_proto3_upb_rust_proto",
        "//rust/test:unittest_upb_rust_proto",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "no_internal_access_test_cpp",
    srcs = ["no_internal_access_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_cpp_export",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "no_internal_access_test_upb",
    srcs = ["no_internal_access_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        "//rust:protobuf_upb_export",
        "@crate_index//:googletest",
    ],
)
