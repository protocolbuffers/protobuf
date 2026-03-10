# Copyright (c) 2009-2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//upb/bazel:copts.bzl", "UPB_DEFAULT_COPTS")

package(default_applicable_licenses = ["//:license"])

# Our preprocessor code does not work with the default (buggy) MSVC preprocessor.
# TODO: Remove this once the new preprocessor is enabled by default.
FASTTABLE_COPTS = UPB_DEFAULT_COPTS + select({
    "//build_defs:config_msvc": ["/Zc:preprocessor"],
    "//conditions:default": [],
})

bool_flag(
    name = "debug_trace",
    build_setting_default = False,
)

config_setting(
    name = "debug_trace_setting",
    flag_values = {":debug_trace": "True"},
)

cc_library(
    name = "field_parsers",
    srcs = [
        "cardinality.c",
        "cardinality.h",
        "field_fixed.c",
        "field_message.c",
        "field_string.c",
        "field_varint.c",
    ],
    hdrs = ["field_parsers.h"],
    copts = FASTTABLE_COPTS,
    defines = select({
        ":debug_trace_setting": ["UPB_TRACE_FASTDECODER=1"],
        "//conditions:default": [],
    }),
    visibility = ["//upb:__pkg__"],
    deps = [
        ":combinations",
        ":data",
        ":dispatch",
        "//upb/base",
        "//upb/base:internal",
        "//upb/mem",
        "//upb/message",
        "//upb/message:internal",
        "//upb/message:types",
        "//upb/mini_table",
        "//upb/port",
        "//upb/wire:decoder",
        "//upb/wire:eps_copy_input_stream",
        "//upb/wire:reader",
    ],
)

cc_library(
    name = "dispatch",
    srcs = ["dispatch.c"],
    hdrs = ["dispatch.h"],
    copts = FASTTABLE_COPTS,
    visibility = ["//upb/wire:__pkg__"],
    deps = [
        "//upb/message",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/wire:decoder",
        "//upb/wire:eps_copy_input_stream",
    ],
)

cc_library(
    name = "select",
    srcs = ["select.c"],
    hdrs = ["select.h"],
    copts = FASTTABLE_COPTS,
    visibility = [
        "//pkg:__pkg__",
        "//upb/mini_descriptor:__pkg__",
        "//upb_generator/minitable:__pkg__",
    ],
    deps = [
        ":combinations",
        ":data",
        "//upb/base",
        "//upb/base:internal",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/wire:reader",
        "@abseil-cpp//absl/strings",
    ],
)

cc_test(
    name = "select_test",
    srcs = ["select_test.cc"],
    deps = [
        ":combinations",
        ":select",
        "@abseil-cpp//absl/base:core_headers",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "combinations",
    hdrs = ["combinations.h"],
    copts = FASTTABLE_COPTS,
    visibility = [
        "//upb:__pkg__",
        "//upb/wire:__subpackages__",
    ],
    deps = [
        "//upb/base:internal",
        "//upb/port",
        "//upb/wire:reader",
    ],
)

cc_library(
    name = "data",
    hdrs = ["data.h"],
    copts = FASTTABLE_COPTS,
    visibility = [
        "//upb:__pkg__",
        "//upb/wire:__subpackages__",
    ],
    deps = [
        "//upb/port",
    ],
)

cc_library(
    name = "function_array",
    srcs = ["function_array.c"],
    hdrs = ["function_array.h"],
    copts = FASTTABLE_COPTS,
    visibility = [
        "//upb:__pkg__",
        "//upb/mini_descriptor:__pkg__",
    ],
    deps = [
        ":combinations",
        ":field_parsers",
        "//upb/mini_table:internal",
        "//upb/port",
    ],
)

cc_binary(
    name = "bisect_helper",
    srcs = ["bisect_helper.cc"],
    deps = [
        ":select",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
    ],
)
