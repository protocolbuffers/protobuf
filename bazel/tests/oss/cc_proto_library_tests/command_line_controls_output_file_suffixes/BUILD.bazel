load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

cc_proto_library(
    name = "foo_cc_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

bzl_test_suite(
    name = "test_command_line_controls_output_file_suffixes",
    config_settings = {
        "@@//bazel/flags/cc:cc_proto_library_header_suffixes": [
            ".pb.h",
            ".proto.h",
        ],
        "@@//bazel/flags/cc:cc_proto_library_source_suffixes": [
            ".pb.cc",
            ".pb.cc.meta",
        ],
        "//command_line_option:features": [
            "supports_dynamic_linker",
            "supports_interface_shared_libraries",
        ],
    },
    tests = TESTS,
)
