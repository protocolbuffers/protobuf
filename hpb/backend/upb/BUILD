# Copyright (c) 2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "upb",
    hdrs = ["upb.h"],
    visibility = ["//upb:friends"],
    deps = [
        ":interop",
        "//hpb:arena",
        "//hpb:extension",
        "//hpb:options",
        "//hpb:ptr",
        "//hpb:status",
        "//hpb/backend:types",
        "//hpb/internal",
        "//hpb/internal:message_lock",
        "//hpb/internal:template_help",
        "//upb/mem",
        "//upb/message",
        "//upb/port",
        "//upb/wire",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "interop",
    hdrs = ["interop.h"],
    visibility = [
        "//hpb:__subpackages__",
        "//hpb_generator:__subpackages__",
    ],
    deps = [
        "//hpb:ptr",
        "//hpb/internal",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/mini_table",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_test(
    name = "interop_test",
    srcs = ["interop_test.cc"],
    deps = [
        ":interop",
        "//hpb_generator/tests:test_model_hpb_proto",
        "//hpb_generator/tests:test_model_upb_minitable_proto",
        "//hpb_generator/tests:test_model_upb_proto",
        "//upb/mem",
        "//upb/message",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "extension",
    srcs = ["extension.cc"],
    hdrs = ["extension.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        ":interop",
        "//hpb:status",
        "//hpb/internal:message_lock",
        "//hpb/internal:template_help",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/mini_table",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "repeated_field_iterator",
    hdrs = ["repeated_field_iterator.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        ":interop",
        "//hpb",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_test(
    name = "repeated_field_iterator_test",
    srcs = ["repeated_field_iterator_test.cc"],
    deps = [
        ":interop",
        ":repeated_field_iterator",
        "//hpb:arena",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "repeated_field",
    hdrs = ["repeated_field.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        ":interop",
        ":repeated_field_iterator",
        "//hpb",
        "//hpb:multibackend",
        "//hpb/internal:template_help",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/message:copy",
    ],
)

cc_library(
    name = "error",
    hdrs = ["error.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//upb/wire",
        "@abseil-cpp//absl/log:absl_log",
    ],
)
