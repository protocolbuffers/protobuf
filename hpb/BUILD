# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//hpb/bazel:hpb_proto_library.bzl",
    "hpb_proto_library_copts",
)
load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

cc_library(
    name = "hpb",
    srcs = [
        "hpb.cc",
    ],
    hdrs = [
        "hpb.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    defines = ["HPB_BACKEND_UPB"],
    visibility = ["//visibility:public"],
    deps = [
        "//hpb:arena",
        "//hpb:extension",
        "//hpb:ptr",
        "//hpb/backend/upb",
        "//hpb/backend/upb:interop",
        "//hpb/internal",
        "//hpb/internal:message_lock",
        "//hpb/internal:template_help",
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:message_copy",
        "//upb:message_promote",
        "//upb:mini_table",
        "//upb:wire",
        "//upb:wire_reader",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "repeated_field",
    hdrs = [
        "repeated_field.h",
        "repeated_field_iterator.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":hpb",
        "//hpb/backend/upb:interop",
        "//hpb/internal:template_help",
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:message_copy",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
    ],
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_hpb_support",
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//hpb/bazel:__pkg__"],
    deps = [
        ":hpb",
        ":repeated_field",
        "//upb:mem",
        "//upb:message",
    ],
)

hpb_proto_library_copts(
    name = "hpb_proto_library_copts",
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//hpb/bazel:__pkg__"],
)

cc_test(
    name = "repeated_field_iterator_test",
    srcs = ["repeated_field_iterator_test.cc"],
    deps = [
        ":repeated_field",
        "//hpb",
        "//upb:message",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "requires",
    hdrs = ["requires.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ptr",
    hdrs = ["ptr.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "extension",
    srcs = ["extension.cc"],
    hdrs = ["extension.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//upb:mem",
        "//upb:mini_table",
    ],
)

cc_library(
    name = "arena",
    hdrs = ["arena.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//upb:mem",
    ],
)
