# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
    "upb_proto_reflection_library",
)
load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

cc_library(
    name = "parse_text_proto",
    testonly = 1,
    hdrs = ["parse_text_proto.h"],
    visibility = ["//upb:__subpackages__"],
    deps = [
        "//:protobuf",
        "@com_google_googletest//:gtest",
    ],
)

proto_library(
    name = "empty_proto",
    srcs = ["empty.proto"],
)

upb_proto_reflection_library(
    name = "empty_upb_proto_reflection",
    testonly = 1,
    deps = [":empty_proto"],
)

proto_library(
    name = "proto3_test_proto",
    testonly = 1,
    srcs = ["proto3_test.proto"],
    deps = ["//:descriptor_proto"],
)

upb_c_proto_library(
    name = "proto3_test_upb_proto",
    testonly = 1,
    deps = [":proto3_test_proto"],
)

upb_proto_reflection_library(
    name = "proto3_test_upb_proto_reflection",
    testonly = 1,
    deps = [":proto3_test_proto"],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["test.proto"],
    visibility = ["//upb:__subpackages__"],
)

upb_minitable_proto_library(
    name = "test_proto_upb_minitable",
    testonly = 1,
    visibility = ["//upb:__subpackages__"],
    deps = [":test_proto"],
)

upb_c_proto_library(
    name = "test_upb_proto",
    testonly = 1,
    visibility = ["//upb:__subpackages__"],
    deps = [":test_proto"],
)

proto_library(
    name = "editions_test_proto",
    testonly = 1,
    srcs = ["editions_test.proto"],
)

upb_c_proto_library(
    name = "editions_test_upb_c_proto",
    testonly = 1,
    deps = [":editions_test_proto"],
)

upb_proto_reflection_library(
    name = "editions_test_upb_proto_reflection",
    testonly = 1,
    deps = [":editions_test_proto"],
)

proto_library(
    name = "test_cpp_proto",
    srcs = ["test_cpp.proto"],
    deps = ["//:timestamp_proto"],
)

upb_c_proto_library(
    name = "test_cpp_upb_proto",
    deps = ["test_cpp_proto"],
)

upb_proto_reflection_library(
    name = "test_cpp_upb_proto_reflection",
    deps = ["test_cpp_proto"],
)

upb_minitable_proto_library(
    name = "test_messages_proto2_upb_minitable",
    testonly = 1,
    visibility = ["//upb:__subpackages__"],
    deps = ["//src/google/protobuf:test_messages_proto2_proto"],
)

upb_minitable_proto_library(
    name = "test_messages_proto3_upb_minitable",
    testonly = 1,
    visibility = ["//upb:__subpackages__"],
    deps = ["//src/google/protobuf:test_messages_proto3_proto"],
)

upb_c_proto_library(
    name = "test_messages_proto2_upb_proto",
    testonly = 1,
    visibility = ["//upb:__subpackages__"],
    deps = ["//src/google/protobuf:test_messages_proto2_proto"],
)

upb_c_proto_library(
    name = "test_messages_proto3_upb_proto",
    testonly = 1,
    visibility = ["//upb:__subpackages__"],
    deps = ["//src/google/protobuf:test_messages_proto3_proto"],
)

upb_c_proto_library(
    name = "timestamp_upb_proto",
    deps = ["//:timestamp_proto"],
)

upb_proto_reflection_library(
    name = "timestamp_upb_proto_reflection",
    deps = ["//:timestamp_proto"],
)

cc_library(
    name = "fuzz_util",
    testonly = 1,
    srcs = ["fuzz_util.cc"],
    hdrs = ["fuzz_util.h"],
    visibility = ["//upb:__subpackages__"],
    deps = [
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_descriptor",
        "//upb:mini_table",
        "//upb:port",
    ],
)

cc_test(
    name = "proto3_test",
    srcs = ["proto3_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":proto3_test_upb_proto",
        ":proto3_test_upb_proto_reflection",
        "//upb:reflection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "editions_test",
    srcs = ["editions_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":editions_test_upb_c_proto",
        ":editions_test_upb_proto_reflection",
        "//upb:base",
        "//upb:mem",
        "//upb:reflection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "length_prefixed_test",
    srcs = ["length_prefixed_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":test_messages_proto2_upb_minitable",
        ":test_messages_proto2_upb_proto",
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:message_compare",
        "//upb:mini_table",
        "//upb:wire",
        "//upb/mem:internal",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_cpp",
    srcs = ["test_cpp.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":test_cpp_upb_proto",
        ":test_cpp_upb_proto_reflection",
        ":timestamp_upb_proto",
        ":timestamp_upb_proto_reflection",
        "//upb:base",
        "//upb:json",
        "//upb:port",
        "//upb:reflection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_generated_code",
    srcs = [
        "test_generated_code.cc",
    ],
    deps = [
        ":empty_upb_proto_reflection",
        ":test_messages_proto2_upb_proto",
        ":test_messages_proto3_upb_proto",
        ":test_upb_proto",
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:port",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_mini_table_oneof",
    srcs = [
        "test_mini_table_oneof.cc",
    ],
    deps = [
        ":empty_upb_proto_reflection",
        ":proto3_test_upb_proto",
        ":test_messages_proto2_upb_minitable",
        ":test_upb_proto",
        "//upb:mini_table",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "empty_srcs_proto",
    testonly = 1,
    srcs = [],
    deps = [":test_proto"],
)

proto_library(
    name = "empty_srcs_2_proto",
    testonly = 1,
    srcs = [],
    deps = [":test_cpp_proto"],
)

proto_library(
    name = "empty_srcs_3_proto",
    testonly = 1,
    srcs = [],
    deps = [
        ":empty_srcs_2_proto",
        ":empty_srcs_proto",
    ],
)

proto_library(
    name = "test_import_empty_srcs_proto",
    testonly = 1,
    srcs = ["test_import_empty_srcs.proto"],
    deps = [":empty_srcs_3_proto"],
)

upb_minitable_proto_library(
    name = "test_import_empty_srcs_upb_minitable_proto",
    testonly = 1,
    deps = [":test_import_empty_srcs_proto"],
)

upb_c_proto_library(
    name = "test_import_empty_srcs_upb_proto",
    testonly = 1,
    deps = [":test_import_empty_srcs_proto"],
)

cc_test(
    name = "test_import_empty_srcs",
    srcs = ["test_import_empty_srcs.cc"],
    deps = [
        ":test_import_empty_srcs_upb_minitable_proto",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# begin:github_only
filegroup(
    name = "test_srcs",
    srcs = glob(
        [
            "**/*test.cc",
            "**/test_*.cc",
        ],
    ),
    visibility = ["//upb:__pkg__"],
)

filegroup(
    name = "test_util",
    srcs = [
        "fuzz_util.cc",
        "fuzz_util.h",
        "parse_text_proto.h",
    ],
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
# end:github_only
