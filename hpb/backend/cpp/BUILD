# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "cpp",
    hdrs = ["cpp.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//hpb:arena",
        "//hpb:extension",
        "//hpb:ptr",
        "//hpb/internal:template_help",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "interop",
    hdrs = ["interop.h"],
    deps = [
        "//hpb/internal",
    ],
)

cc_library(
    name = "error",
    hdrs = ["error.h"],
    visibility = ["//hpb:__subpackages__"],
)

cc_library(
    name = "repeated_field",
    hdrs = ["repeated_field.h"],
    visibility = ["//hpb:__subpackages__"],
)

cc_test(
    name = "cpp_test",
    srcs = ["cpp_test.cc"],
    deps = [
        "//hpb",
        "//hpb:arena",
        "//hpb:ptr",
        "//hpb_generator/tests:test_model_hpb_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
