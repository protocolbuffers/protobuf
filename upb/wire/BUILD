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
    ],
    hdrs = [
        "decode.h",
        "encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        ":types",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
        "//upb:port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "decode.c",
        "decode.h",
        "decode_fast.c",
        "encode.c",
        "encode.h",
    ],
    hdrs = [
        "decode_fast.h",
        "internal/constants.h",
        "internal/decode.h",
        "internal/swap.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":reader",
        ":types",
        "//upb:base",
        "//upb:hash",
        "//upb:mem",
        "//upb:mem_internal",
        "//upb:message",
        "//upb:message_accessors_internal",
        "//upb:message_internal",
        "//upb:message_internal_types",
        "//upb:message_tagged_ptr",
        "//upb:mini_table",
        "//upb:mini_table_internal",
        "//upb:port",
        "//third_party/utf8_range",
    ],
)

cc_library(
    name = "reader",
    srcs = [
        "internal/swap.h",
        "reader.c",
    ],
    hdrs = ["reader.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":types",
        "//upb:port",
    ],
)

cc_library(
    name = "types",
    hdrs = ["types.h"],
    visibility = ["//visibility:public"],
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
