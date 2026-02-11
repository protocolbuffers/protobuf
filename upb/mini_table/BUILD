# Copyright (c) 2009-2023, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bazel:proto_library.bzl", "proto_library")
load(
    "//upb/bazel:copts.bzl",
    "UPB_DEFAULT_COPTS",
)
load(
    "//upb/bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)
load(
    "//upb/cmake:build_defs.bzl",
    "staleness_test",
)

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "mini_table",
    srcs = [
        "extension_registry.c",
        "generated_registry.c",
        "message.c",
    ],
    hdrs = [
        "enum.h",
        "extension.h",
        "extension_registry.h",
        "field.h",
        "file.h",
        "generated_registry.h",
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
    name = "debug_string",
    srcs = [
        "debug_string.c",
    ],
    hdrs = [
        "debug_string.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = [
        "//rust/upb:__pkg__",
        "//upb:__pkg__",
        "//upb:friends",
    ],
    deps = [
        ":internal",
        ":mini_table",
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
        "internal/generated_registry.h",
        "internal/message.h",
        "internal/size_log2.h",
        "internal/sub.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb/base",
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

cc_test(
    name = "generated_registry_test",
    srcs = ["generated_registry_test.cc"],
    deps = [
        ":mini_table",
        "//src/google/protobuf:descriptor_upb_minitable_proto",
        "//upb/test:custom_options_upb_minitable_proto",
        "//upb/test:editions_test_upb_minitable_proto",
        "//upb/test:test_multiple_files_upb_minitable_proto",
        "@abseil-cpp//absl/synchronization",
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

proto_library(
    name = "debug_string_test_proto",
    srcs = ["debug_string_test.proto"],
)

upb_minitable_proto_library(
    name = "debug_string_test_upb_minitable_proto",
    deps = [":debug_string_test_proto"],
)

cc_binary(
    name = "debug_string_test_generate",
    srcs = ["debug_string_test_generate.cc"],
    deps = [
        ":debug_string",
        ":debug_string_test_upb_minitable_proto",
    ],
)

genrule(
    name = "debug_string_test_txt",
    outs = ["generated/debug_string_test.txt"],
    cmd = "$(location :debug_string_test_generate) > $@",
    tools = [":debug_string_test_generate"],
)

staleness_test(
    name = "debug_string_staleness_test",
    outs = ["debug_string_test.txt"],
    generated_pattern = "generated/%s",
    target_files = ["debug_string_test.txt"],
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
