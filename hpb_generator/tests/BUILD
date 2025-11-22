# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load(
    "//bazel:cc_proto_library.bzl",
    "cc_proto_library",
)
load("//bazel:proto_library.bzl", "proto_library")
load(
    "//bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
)
load(
    "//hpb/bazel:hpb_proto_library.bzl",
    "hpb_proto_library",
)

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

proto_library(
    name = "test_model_proto",
    srcs = [
        "child_model.proto",
        "test_enum.proto",
        "test_extension.proto",
        "test_model.proto",
    ],
)

proto_library(
    name = "no_package_proto",
    srcs = [
        "no_package.proto",
    ],
)

proto_library(
    name = "naming_conflict_proto",
    srcs = [
        "naming_conflict.proto",
    ],
)

proto_library(
    name = "no_package_enum_user_proto",
    srcs = [
        "no_package_enum_user.proto",
    ],
    deps = [":no_package_proto"],
)

upb_c_proto_library(
    name = "test_model_upb_proto",
    visibility = [
        "//hpb:__subpackages__",
    ],
    deps = [":test_model_proto"],
)

upb_minitable_proto_library(
    name = "test_model_upb_minitable_proto",
    visibility = [
        "//hpb:__subpackages__",
    ],
    deps = [":test_model_proto"],
)

hpb_proto_library(
    name = "test_model_hpb_proto",
    visibility = [
        "//hpb:__subpackages__",
    ],
    deps = [":test_model_proto"],
)

hpb_proto_library(
    name = "naming_conflict_hpb_proto",
    visibility = ["//visibility:private"],
    deps = [":naming_conflict_proto"],
)

hpb_proto_library(
    name = "no_package_hpb_proto",
    deps = [
        ":no_package_proto",
    ],
)

hpb_proto_library(
    name = "no_package_enum_user_hpb_proto",
    deps = [
        ":no_package_enum_user_proto",
    ],
)

cc_proto_library(
    name = "test_model_cc_proto",
    deps = [":test_model_proto"],
)

proto_library(
    name = "legacy_name_proto",
    srcs = [
        "legacy-name.proto",
    ],
)

hpb_proto_library(
    name = "legacy_name_test_proto",
    visibility = ["//visibility:private"],
    deps = [":legacy_name_proto"],
)

proto_library(
    name = "basic_editions_proto",
    srcs = [
        "basic_test_editions.proto",
    ],
)

hpb_proto_library(
    name = "basic_test_editions_proto",
    visibility = ["//visibility:private"],
    deps = [":basic_editions_proto"],
)

proto_library(
    name = "set_alias_proto",
    srcs = ["set_alias.proto"],
)

proto_library(
    name = "set_alias_no_srcs_proto",
    deps = [":set_alias_proto"],
)

hpb_proto_library(
    name = "set_alias_hpb_proto",
    deps = [":set_alias_proto"],
)

hpb_proto_library(
    name = "set_alias_no_srcs_hpb_proto",
    deps = [":set_alias_no_srcs_proto"],
)

cc_test(
    name = "test_generated_cc_code",
    srcs = ["test_generated.cc"],
    deps = [
        # google_only_begin_legacy_marker
        ":legacy_name_test_proto",
        # google_only_end_legacy_marker
        ":set_alias_hpb_proto",
        ":no_package_hpb_proto",
        ":test_model_hpb_proto",
        ":test_model_upb_proto",
        ":naming_conflict_hpb_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "//hpb",
        "//hpb/backend/upb:interop",
        "//hpb:arena",
        "//hpb:extension",
        "//hpb:options",
        "//hpb:requires",
        "//hpb:ptr",
        "//hpb:repeated_field",
        "//hpb:status",
        "//upb/mem",
        "//upb/mini_table",
    ],
)

cc_test(
    name = "extension_test",
    srcs = ["extension_test.cc"],
    deps = [
        ":test_model_hpb_proto",
        ":test_model_upb_proto",
        "//hpb",
        "//hpb:arena",
        "//hpb:extension",
        "//hpb:options",
        "//hpb:requires",
        "//hpb:status",
        "//hpb/backend/upb:interop",
        "//upb/mem",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "repeated_test",
    srcs = ["repeated_test.cc"],
    deps = [
        ":test_model_hpb_proto",
        ":test_model_upb_proto",
        "//hpb:arena",
        "//hpb:ptr",
        "//hpb:requires",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_hpb_bzl_alias",
    srcs = ["test_hpb_bzl_alias.cc"],
    deps = [
        ":set_alias_no_srcs_hpb_proto",
        "//hpb",
        "//hpb:arena",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "multibackend_test",
    srcs = ["multibackend_test.cc"],
    deps = [
        ":test_model_hpb_proto",
        "//hpb",
        "//hpb:arena",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
