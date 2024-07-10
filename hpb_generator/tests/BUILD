# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# todo: re-add post migration
# begin:google_only
# load(
#     "//bazel:cc_proto_library.bzl",
#     "cc_proto_library",
# )
# load(
#     "//hpb/bazel:upb_cc_proto_library.bzl",
#     "upb_cc_proto_library",
# )
# load(
#     "//bazel:upb_proto_library.bzl",
#     "upb_c_proto_library",
# )
# load(
#     "//upb/bazel:build_defs.bzl",
#     "UPB_DEFAULT_CPPOPTS",
# )
# end:google_only

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

# todo: re-add post migration
# begin:google_only
# licenses(["notice"])
#
# proto_library(
#     name = "test_model_proto",
#     srcs = [
#         "child_model.proto",
#         "test_enum.proto",
#         "test_extension.proto",
#         "test_model.proto",
#     ],
# )
#
# proto_library(
#     name = "no_package_proto",
#     srcs = [
#         "no_package.proto",
#     ],
# )
#
# proto_library(
#     name = "naming_conflict_proto",
#     srcs = [
#         "naming_conflict.proto",
#     ],
# )
#
# proto_library(
#     name = "no_package_enum_user_proto",
#     srcs = [
#         "no_package_enum_user.proto",
#     ],
#     deps = [":no_package_proto"],
# )
#
# upb_c_proto_library(
#     name = "test_model_upb_proto",
#     visibility = [
#         "//hpb:__pkg__",
#         "//protos:__pkg__",
#     ],
#     deps = [":test_model_proto"],
# )
#
# upb_cc_proto_library(
#     name = "test_model_upb_cc_proto",
#     visibility = [
#         "//hpb:__pkg__",
#         "//protos:__pkg__",
#     ],
#     deps = [":test_model_proto"],
# )
#
# upb_cc_proto_library(
#     name = "naming_conflict_upb_cc_proto",
#     visibility = ["//visibility:private"],
#     deps = [":naming_conflict_proto"],
# )
#
# upb_cc_proto_library(
#     name = "no_package_upb_cc_proto",
#     deps = [
#         ":no_package_proto",
#     ],
# )
#
# upb_cc_proto_library(
#     name = "no_package_enum_user_upb_cc_proto",
#     deps = [
#         ":no_package_enum_user_proto",
#     ],
# )
#
# cc_proto_library(
#     name = "test_model_cc_proto",
#     deps = [":test_model_proto"],
# )
# end:google_only

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
#     visibility = ["//visibility:private"],
#     deps = [":legacy_name_proto"],
# )
#
# proto_library(
#     name = "basic_editions_proto",
#     srcs = [
#         "basic_test_editions.proto",
#     ],
# )
#
# upb_cc_proto_library(
#     name = "basic_test_editions_proto",
#     visibility = ["//visibility:private"],
#     deps = [":basic_editions_proto"],
# )
# end:google_only

# todo: re-add post migration
# begin:google_only
# cc_test(
#     name = "test_generated_cc_code",
#     srcs = ["test_generated.cc"],
#     copts = UPB_DEFAULT_CPPOPTS,
#     deps = [
#         # google_only_begin_legacy_marker
#         ":legacy_name_test_proto",
#         # google_only_end_legacy_marker
#         ":no_package_upb_cc_proto",
#         ":test_model_upb_cc_proto",
#         ":test_model_upb_proto",
#         ":naming_conflict_upb_cc_proto",
#         "@com_google_googletest//:gtest", "@com_google_googletest//:gtest_main",
#         "@com_google_absl//absl/status:statusor",
#         "@com_google_absl//absl/strings",
#         "//hpb:requires",
#         "//protos",
#         "//upb:mem",
#         "//protos:repeated_field",
#     ],
# )
# end:google_only
