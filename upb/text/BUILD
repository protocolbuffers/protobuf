# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "text",
    srcs = [
        "encode.c",
    ],
    hdrs = [
        "encode.h",
        "options.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb/base",
        "//upb/hash",
        "//upb/lex",
        "//upb/message",
        "//upb/message:internal",
        "//upb/message:types",
        "//upb/port",
        "//upb/reflection",
        "//upb/wire:eps_copy_input_stream",
    ],
)

cc_library(
    name = "debug",
    srcs = [
        "debug_string.c",
    ],
    hdrs = [
        "debug_string.h",
        "options.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb/base",
        "//upb/lex",
        "//upb/message",
        "//upb/message:internal",
        "//upb/message:iterator",
        "//upb/message:types",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "internal/encode.c",
    ],
    hdrs = [
        "internal/encode.h",
        "options.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//third_party/utf8_range",
        "//upb/base",
        "//upb/lex",
        "//upb/message",
        "//upb/message:internal",
        "//upb/port",
        "//upb/wire:eps_copy_input_stream",
        "//upb/wire:reader",
    ],
)

cc_test(
    name = "encode_debug_test",
    srcs = [
        "encode_debug_test.cc",
    ],
    deps = [
        ":debug",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/message:internal",
        "//upb/mini_table",
        "//upb/port",
        "//upb/test:test_proto_upb_minitable",
        "//upb/test:test_upb_proto",
        "//upb/wire:eps_copy_input_stream",
        "//upb/wire:reader",
        "@abseil-cpp//absl/log:absl_log",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

filegroup(
    name = "source_files",
    srcs = glob(
        [
            "**/*.c",
            "**/*.h",
        ],
    ),
    visibility = [
        "//python/dist:__pkg__",
        "//upb/cmake:__pkg__",
    ],
)
