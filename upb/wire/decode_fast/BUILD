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
load("//upb/bazel:copts.bzl", "UPB_DEFAULT_COPTS", "UPB_DEFAULT_CPPOPTS", "UPB_DEFAULT_FEATURES")

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
        "field_extension.c",
        "field_fixed.c",
        "field_generic.c",
        "field_helpers.h",
        "field_message.c",
        "field_mismatch.c",
        "field_string.c",
        "field_unknown.c",
        "field_varint.c",
    ],
    hdrs = ["field_parsers.h"],
    copts = FASTTABLE_COPTS,
    defines = select({
        ":debug_trace_setting": ["UPB_TRACE_FASTDECODER=1"],
        "//conditions:default": [],
    }),
    features = UPB_DEFAULT_FEATURES,
    visibility = [
        "//upb:__pkg__",
        "//upb/mini_descriptor:__pkg__",
    ],
    deps = [
        ":combinations",
        ":data",
        ":dispatch",
        ":select",
        "//third_party/utf8_range",
        "//upb/base",
        "//upb/base:internal",
        "//upb/mem",
        "//upb/message",
        "//upb/message:internal",
        "//upb/message:types",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/wire",
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
    features = UPB_DEFAULT_FEATURES,
    visibility = ["//upb/wire:__pkg__"],
    deps = [
        "//upb/message",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/wire:decoder",
        "//upb/wire:eps_copy_input_stream",
        "//upb/wire:eps_copy_input_stream_internal",
    ],
)

cc_library(
    name = "select",
    srcs = ["select.c"],
    hdrs = ["select.h"],
    copts = FASTTABLE_COPTS,
    features = UPB_DEFAULT_FEATURES,
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
    ],
)

cc_test(
    name = "select_test",
    srcs = ["select_test.cc"],
    deps = [
        ":combinations",
        ":select",
        "//upb/mem",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/wire",  # buildcleaner: keep
        "//upb/wire/test_util:field_types",
        "//upb/wire/test_util:make_mini_table",
        "@abseil-cpp//absl/base:core_headers",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "combinations",
    hdrs = ["combinations.h"],
    copts = FASTTABLE_COPTS,
    features = UPB_DEFAULT_FEATURES,
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
    features = UPB_DEFAULT_FEATURES,
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
    features = UPB_DEFAULT_FEATURES,
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

cc_test(
    name = "test_unknown",
    srcs = ["test_unknown.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    features = UPB_DEFAULT_FEATURES,
    deps = [
        ":combinations",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/port",
        "//upb/wire",
        "//upb/wire/test_util:field_types",
        "//upb/wire/test_util:make_mini_table",
        "//upb/wire/test_util:wire_message",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
