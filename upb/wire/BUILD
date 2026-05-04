# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//upb/bazel:copts.bzl", "UPB_DEFAULT_COPTS", "UPB_DEFAULT_CPPOPTS", "UPB_DEFAULT_FEATURES")
load("//upb/bazel:upb_proto_library.bzl", "upb_proto_library")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "decoder",
    srcs = [
        "internal/decoder.c",
    ],
    hdrs = [
        "decode.h",
        "internal/decoder.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    features = UPB_DEFAULT_FEATURES,
    visibility = [
        "//upb:__pkg__",
        "//upb/wire/decode_fast:__subpackages__",
    ],
    deps = [
        ":eps_copy_input_stream",
        "//third_party/utf8_range",
        "//upb/base",
        "//upb/mem",
        "//upb/mem:internal",
        "//upb/message",
        "//upb/message:internal",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
    ],
)

cc_library(
    name = "wire",
    srcs = [
        "decode.c",
        "encode.c",
        "internal/constants.h",
    ],
    hdrs = [
        "decode.h",
        "encode.h",
        "internal/encoder.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    features = UPB_DEFAULT_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        ":decoder",
        ":eps_copy_input_stream",
        ":reader",
        ":writer",
        "//third_party/utf8_range",
        "//upb/base",
        "//upb/base:internal",
        "//upb/hash",
        "//upb/mem",
        "//upb/mem:internal",
        "//upb/message",
        "//upb/message:internal",
        "//upb/message:iterator",
        "//upb/message:types",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
    ] + select({
        "//upb:fasttable_enabled_setting": [
            "//upb/wire/decode_fast:dispatch",
        ],
        "//conditions:default": [],
    }),
)

cc_test(
    name = "decode_test",
    srcs = ["decode_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    features = UPB_DEFAULT_FEATURES,
    deps = [
        ":wire",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/message:message_cc",
        "//upb/mini_descriptor",
        "//upb/mini_table",
        "//upb/port",
        "//upb/wire/decode_fast:combinations",
        "//upb/wire/test_util:field_types",
        "//upb/wire/test_util:make_mini_table",
        "//upb/wire/test_util:wire_message",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

proto_library(
    name = "encode_test_proto",
    srcs = ["encode_test.proto"],
)

upb_proto_library(
    name = "encode_test_upb_proto",
    deps = [":encode_test_proto"],
)

cc_test(
    name = "encode_test",
    srcs = ["encode_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    features = UPB_DEFAULT_FEATURES,
    deps = [
        ":encode_test_upb_proto",
        ":wire",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/message:internal",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "decode_benchmark",
    testonly = True,
    srcs = ["decode_benchmark.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    features = UPB_DEFAULT_FEATURES,
    deps = [
        ":wire",
        "//upb/mem",
        "//upb/message",
        "//upb/mini_table",
        "//upb/port",
        "//upb/wire/decode_fast:combinations",
        "//upb/wire/test_util:field_types",
        "//upb/wire/test_util:make_mini_table",
        "//upb/wire/test_util:wire_message",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@google_benchmark//:benchmark_main",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "reader_internal",
    hdrs = ["internal/reader.h"],
    copts = UPB_DEFAULT_COPTS,
    features = UPB_DEFAULT_FEATURES,
    visibility = ["//upb:__pkg__"],
    deps = [
        ":eps_copy_input_stream",
        "//upb/port",
    ],
)

cc_library(
    name = "reader",
    srcs = ["reader.c"],
    hdrs = [
        "reader.h",
        "types.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    features = UPB_DEFAULT_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":reader_internal",
        "//upb/base:internal",
        "//upb/port",
    ],
)

cc_library(
    name = "writer",
    hdrs = [
        "writer.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    features = UPB_DEFAULT_FEATURES,
    visibility = [
        "//upb:__pkg__",
        "//upb/message:__pkg__",
    ],
    deps = [
        "//upb/port",
    ],
)

cc_library(
    name = "byte_size",
    srcs = ["byte_size.c"],
    hdrs = ["byte_size.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":wire",
        "//upb/mem",
        "//upb/message",
        "//upb/mini_table",
        "//upb/port",
    ],
)

cc_test(
    name = "byte_size_test",
    srcs = ["byte_size_test.cc"],
    deps = [
        ":byte_size",
        "//upb/base",
        "//upb/mem",
        "//upb/mini_table",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto2_upb_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "eps_copy_input_stream_internal",
    hdrs = ["internal/eps_copy_input_stream.h"],
    visibility = [
        "//upb:__pkg__",
        "//upb/wire/decode_fast:__subpackages__",
    ],
    deps = [
        "//upb/base",
        "//upb/mem",
        "//upb/port",
    ],
)

cc_library(
    name = "eps_copy_input_stream",
    srcs = ["eps_copy_input_stream.c"],
    hdrs = ["eps_copy_input_stream.h"],
    copts = UPB_DEFAULT_COPTS,
    features = UPB_DEFAULT_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream_internal",
        "//upb/base",
        "//upb/mem",
        "//upb/port",
    ],
)

cc_test(
    name = "eps_copy_input_stream_test",
    srcs = ["eps_copy_input_stream_test.cc"],
    deps = [
        ":eps_copy_input_stream",
        "//upb/base",
        "//upb/mem",
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

filegroup(
    name = "test_protos",
    srcs = glob(
        [
            "**/*.proto",
        ],
    ),
    visibility = ["//upb:__pkg__"],
)
