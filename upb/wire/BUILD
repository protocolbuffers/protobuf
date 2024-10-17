# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "wire",
    srcs = [
        "decode.c",
        "encode.c",
        "internal/constants.h",
        "internal/decode_fast.c",
        "internal/decoder.h",
    ],
    hdrs = [
        "decode.h",
        "encode.h",
        "internal/decode_fast.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":reader",
        "//third_party/utf8_range",
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
        "//upb:port",
        "//upb/base:internal",
        "//upb/hash",
        "//upb/mem:internal",
        "//upb/message:internal",
        "//upb/message:types",
        "//upb/mini_table:internal",
    ],
)

cc_library(
    name = "reader",
    srcs = [
        "internal/reader.h",
        "reader.c",
    ],
    hdrs = [
        "reader.h",
        "types.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        "//upb:port",
        "//upb/base:internal",
    ],
)

cc_library(
    name = "byte_size",
    srcs = ["byte_size.c"],
    hdrs = ["byte_size.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire",
    ],
)

cc_test(
    name = "byte_size_test",
    srcs = ["byte_size_test.cc"],
    deps = [
        ":byte_size",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto2_upb_proto",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "eps_copy_input_stream",
    srcs = ["eps_copy_input_stream.c"],
    hdrs = ["eps_copy_input_stream.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//upb:mem",
        "//upb:port",
    ],
)

cc_test(
    name = "eps_copy_input_stream_test",
    srcs = ["eps_copy_input_stream_test.cc"],
    deps = [
        ":eps_copy_input_stream",
        "//upb:mem",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
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
