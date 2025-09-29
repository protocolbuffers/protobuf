# Copyright (c) 2009-2023, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("//bazel:proto_library.bzl", "proto_library")
load(
    "//bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)
load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "mini_table",
    srcs = [
        "extension_registry.c",
        "message.c",
    ],
    hdrs = [
        "enum.h",
        "extension.h",
        "extension_registry.h",
        "field.h",
        "file.h",
        "message.h",
        "sub.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb/base",
        "//upb/hash",
        "//upb/mem",
        "//upb/port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "internal/message.c",
    ],
    hdrs = [
        "internal/enum.h",
        "internal/extension.h",
        "internal/field.h",
        "internal/file.h",
        "internal/message.h",
        "internal/size_log2.h",
        "internal/sub.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb/base",
        "//upb/hash",
        "//upb/mem",
        "//upb/message:types",
        "//upb/port",
    ],
)

cc_library(
    name = "compat",
    srcs = [
        "compat.c",
    ],
    hdrs = [
        "compat.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":mini_table",
        "//upb/base",
        "//upb/hash",
        "//upb/mem",
        "//upb/port",
    ],
)

cc_test(
    name = "compat_test",
    srcs = ["compat_test.cc"],
    deps = [
        ":compat",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto3_upb_minitable",
        "//upb/test:test_upb_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

proto_library(
    name = "message_benchmark_proto",
    testonly = 1,
    srcs = ["message_benchmark.proto"],
)

upb_minitable_proto_library(
    name = "message_benchmark_upb_minitable_proto",
    testonly = 1,
    deps = [":message_benchmark_proto"],
)

cc_binary(
    name = "message_benchmark",
    testonly = 1,
    srcs = ["message_benchmark.cc"],
    deps = [
        ":compat",
        ":message_benchmark_upb_minitable_proto",
        ":mini_table",
        "//upb/port",
        "@abseil-cpp//absl/random",
        "@google_benchmark//:benchmark_main",
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

filegroup(
    name = "test_srcs",
    srcs = glob(
        [
            "**/*test.cc",
        ],
    ),
    visibility = ["//upb:__pkg__"],
)
