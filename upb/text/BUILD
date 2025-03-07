# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

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
        "//upb:base",
        "//upb:eps_copy_input_stream",
        "//upb:message",
        "//upb:port",
        "//upb:reflection",
        "//upb/hash",
        "//upb/lex",
        "//upb/message:internal",
        "//upb/message:types",
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
        "//upb:base",
        "//upb:message",
        "//upb:mini_table",
        "//upb:port",
        "//upb/lex",
        "//upb/message:internal",
        "//upb/message:iterator",
        "//upb/message:types",
        "//upb/mini_table:internal",
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
        "//upb:base",
        "//upb:eps_copy_input_stream",
        "//upb:message",
        "//upb:port",
        "//upb:wire_reader",
        "//upb/lex",
        "//upb/message:internal",
    ],
)

cc_test(
    name = "encode_debug_test",
    srcs = [
        "encode_debug_test.cc",
    ],
    deps = [
        ":debug",
        "//upb:base",
        "//upb:eps_copy_input_stream",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire_reader",
        "//upb/message:internal",
        "//upb/test:test_proto_upb_minitable",
        "//upb/test:test_upb_proto",
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
