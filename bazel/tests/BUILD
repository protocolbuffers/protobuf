load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("//bazel:proto_descriptor_set.bzl", "proto_descriptor_set")
load("//bazel:proto_library.bzl", "proto_library")
load(":bazel_proto_library_tests.bzl", "bazel_proto_library_test_suite")
load(":cc_proto_library_tests.bzl", "cc_proto_library_test_suite")
load(":cc_toolchain_tests.bzl", "cc_toolchain_test_suite")
load(":java_proto_library_tests.bzl", "java_proto_library_test_suite")
load(":proto_common_check_collocated_tests.bzl", "proto_common_check_collocated_test_suite")
load(":proto_common_compile_tests.bzl", "proto_common_compile_test_suite")
load(":proto_common_declare_generated_files_tests.bzl", "proto_common_declare_generated_files_test_suite")
load(":proto_common_should_generate_tests.bzl", "proto_common_should_generate_test_suite")
load(":py_proto_library_tests.bzl", "py_proto_library_test_suite")

package(default_applicable_licenses = ["//:license"])

proto_common_compile_test_suite(name = "proto_common_compile_test_suite")

proto_common_should_generate_test_suite(name = "proto_common_should_generate_test_suite")

proto_common_declare_generated_files_test_suite(name = "proto_common_declare_generated_files_test_suite")

proto_common_check_collocated_test_suite(name = "proto_common_check_collocated_test_suite")

bazel_proto_library_test_suite(name = "bazel_proto_library_test_suite")

cc_toolchain_test_suite(name = "cc_toolchain_test_suite")

java_proto_library_test_suite(name = "java_proto_library_test_suite")

py_proto_library_test_suite(name = "py_proto_library_test_suite")

cc_proto_library_test_suite(name = "cc_proto_library_tests")

proto_library(
    name = "empty_proto_library",
)

proto_descriptor_set(
    name = "no_protos",
    deps = [
        ":empty_proto_library",
    ],
)

proto_descriptor_set(
    name = "well_known_protos",
    deps = [
        "//:any_proto",
        "//:api_proto",
        "//:compiler_plugin_proto",
        "//:descriptor_proto",
        "//:duration_proto",
        "//:empty_proto",
        "//:field_mask_proto",
        "//:source_context_proto",
        "//:struct_proto",
        "//:timestamp_proto",
        "//:type_proto",
        "//:wrappers_proto",
    ],
)

cc_test(
    name = "proto_descriptor_set_test",
    srcs = [
        "proto_descriptor_set_test.cc",
    ],
    data = [
        ":no_protos",
        ":well_known_protos",
    ],
    deps = [
        "//:protobuf",
        "@bazel_tools//tools/cpp/runfiles",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
