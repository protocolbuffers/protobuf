# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")
load("//bazel:upb_c_proto_library.bzl", "upb_c_proto_library")
load("//bazel:upb_proto_reflection_library.bzl", "upb_proto_reflection_library")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")

cc_library(
    name = "text",
    srcs = [
        "encode.c",
    ],
    hdrs = [
        "encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//third_party/utf8_range",
        "//upb:base",
        "//upb:eps_copy_input_stream",
        "//upb:message",
        "//upb:port",
        "//upb:reflection",
        "//upb:wire_reader",
        "//upb/lex",
        "//upb/message:internal",
        "//upb/message:types",
    ],
)

proto_library(
    name = "encode_test_proto",
    srcs = ["encode_test.proto"],
)

upb_c_proto_library(
    name = "encode_test_upb_c_proto",
    deps = [":encode_test_proto"],
)

upb_proto_reflection_library(
    name = "encode_test_upb_reflection_proto",
    deps = [":encode_test_proto"],
)

cc_proto_library(
    name = "encode_test_cc_proto",
    deps = [":encode_test_proto"],
)

cc_test(
    name = "encode_test",
    srcs = ["encode_test.cc"],
    deps = [
        ":encode_test_cc_proto",
        ":encode_test_upb_c_proto",
        ":encode_test_upb_reflection_proto",
        ":text",
        "//:protobuf",
        "//testing/fuzzing:fuzztest",
        "//upb:base",
        "//upb:mem",
        "//upb:reflection",
        "@com_google_absl//absl/strings:string_view",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
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
        "//python/dist:__pkg__",
        "//upb/cmake:__pkg__",
    ],
)
# end:github_only
