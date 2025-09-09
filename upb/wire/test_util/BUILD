# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_CPPOPTS")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "make_mini_table",
    testonly = True,
    srcs = ["make_mini_table.cc"],
    hdrs = ["make_mini_table.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = [
        "//upb:__pkg__",
        "//upb:__subpackages__",
    ],
    deps = [
        "//upb/base",
        "//upb/mem",
        "//upb/mini_descriptor",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table",
        "//upb/port",
        "//upb/wire:reader",
        "//upb/wire/decode_fast:combinations",
        "//upb/wire/decode_fast:data",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:absl_check",
    ],
)

cc_library(
    name = "field_types",
    testonly = True,
    hdrs = ["field_types.h"],
    visibility = [
        "//upb:__pkg__",
        "//upb:__subpackages__",
    ],
    deps = [
        ":wire_message",
        "//upb/base",
        "//upb/wire/decode_fast:combinations",
        "@abseil-cpp//absl/base",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "wire_message",
    testonly = True,
    srcs = ["wire_message.cc"],
    hdrs = ["wire_message.h"],
    visibility = [
        "//upb:__pkg__",
        "//upb:__subpackages__",
    ],
    deps = [
        "//upb/base:internal",
        "//upb/wire:reader",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings:string_view",
    ],
)
