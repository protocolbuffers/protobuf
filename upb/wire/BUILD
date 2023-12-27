# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

cc_library(
    name = "wire",
    srcs = [
        "decode.c",
        "encode.c",
        "internal/decode_fast.c",
    ],
    hdrs = [
        "decode.h",
        "encode.h",
        "internal/constants.h",
        "internal/decode.h",
        "internal/decode_fast.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":reader",
        "//upb:base",
        "//upb:hash",
        "//upb:mem",
        "//upb:message",
        "//upb:message_accessors",
        "//upb:message_tagged_ptr",
        "//upb:message_types",
        "//upb:mini_table",
        "//upb:port",
        "//upb/mem:internal",
        "//upb/mini_table:internal",
        "//third_party/utf8_range",
    ],
)

cc_library(
    name = "reader",
    srcs = [
        "reader.c",
    ],
    hdrs = [
        "internal/endian.h",
        "internal/reader.h",
        "reader.h",
        "types.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        "//upb:port",
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
        "@com_google_googletest//:gtest_main",
        "//upb:mem",
    ],
)

# begin:github_only
filegroup(
    name = "source_files",
    srcs = glob(
        [
            "**/*.c",
            "**/*.h",
        ],
    ),
    visibility = [
        "//upb/cmake:__pkg__",
        "//python/dist:__pkg__",
    ]
)
# end:github_only

# begin:github_only
filegroup(
    name = "test_srcs",
    srcs = glob(
        [
            "**/*test.cc",
        ],
    ),
    visibility = ["//pkg:__pkg__"],
)
# end:github_only
