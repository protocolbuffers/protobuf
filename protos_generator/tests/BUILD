# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
)
load(
    "//protos/bazel:upb_cc_proto_library.bzl",
    "upb_cc_proto_library",
)
load(
    "@rules_cc//cc:defs.bzl",
    "cc_proto_library",
)

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

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
        "//protos:__pkg__",
    ],
    deps = [":test_model_proto"],
)

upb_cc_proto_library(
    name = "test_model_upb_cc_proto",
    visibility = ["//protos:__pkg__"],
    deps = [":test_model_proto"],
)

upb_cc_proto_library(
    name = "naming_conflict_upb_cc_proto",
    visibility = [
        "//visibility:private",  # Only private by automation, not intent. Owner may accept CLs adding visibility. See go/scheuklappen#explicit-private.
    ],
    deps = [":naming_conflict_proto"],
)

upb_cc_proto_library(
    name = "no_package_upb_cc_proto",
    deps = [
        ":no_package_proto",
    ],
)

upb_cc_proto_library(
    name = "no_package_enum_user_upb_cc_proto",
    deps = [
        ":no_package_enum_user_proto",
    ],
)

cc_proto_library(
    name = "test_model_cc_proto",
    deps = [":test_model_proto"],
)

# begin:google_only
# proto_library(
#     name = "legacy_name_proto",
#     srcs = [
#         "legacy-name.proto",
#     ],
# )
#
# upb_cc_proto_library(
#     name = "legacy_name_test_proto",
#     visibility = [
#         "//visibility:private",  # Only private by automation, not intent. Owner may accept CLs adding visibility. See go/scheuklappen#explicit-private.
#     ],
#     deps = [":legacy_name_proto"],
# )
# end:google_only

cc_test(
    name = "test_generated_cc_code",
    srcs = ["test_generated.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        # begin:google_only
#         ":legacy_name_test_proto",
        # end:google_only
        ":no_package_upb_cc_proto",
        ":test_model_upb_cc_proto",
        ":test_model_upb_proto",
        ":naming_conflict_upb_cc_proto",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "//protos",
        "//upb:mem",
        "//protos:repeated_field",
    ],
)
