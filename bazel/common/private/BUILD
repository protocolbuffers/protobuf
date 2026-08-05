load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load(
    ":proto_path_mapping_test.bzl",
    "proto_path_mapping_test_suite",
)

package(default_applicable_licenses = ["//:license"])

bzl_library(
    name = "proto_path_mapping_test_bzl",
    srcs = ["proto_path_mapping_test.bzl"],
    visibility = ["//visibility:private"],
    deps = [
        "//bazel/common:proto_path_mapping_bzl",
        "@bazel_skylib//lib:unittest",
    ],
)

proto_path_mapping_test_suite()
