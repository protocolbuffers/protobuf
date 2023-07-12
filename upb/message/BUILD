# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")
load(
    "//bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

cc_library(
    name = "accessors",
    srcs = [
        "accessors.c",
        "internal/accessors.h",
    ],
    hdrs = [
        "accessors.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        ":message",
        "//upb:base",
        "//upb:eps_copy_input_stream",
        "//upb:mini_table",
        "//upb:mini_table_internal",
        "//upb:port",
        "//upb:wire",
        "//upb:wire_reader",
    ],
)

cc_library(
    name = "accessors_internal",
    hdrs = [
        "internal/accessors.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb:mini_table_internal",
        "//upb:port",
    ],
)

cc_library(
    name = "copy",
    srcs = [
        "copy.c",
    ],
    hdrs = [
        "copy.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":accessors",
        ":internal",
        ":message",
        ":types",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:mini_table_internal",
        "//upb:port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "array.c",
        "array.h",
        "map.c",
        "map.h",
        "map_sorter.c",
        "message.c",
        "message.h",
    ],
    hdrs = [
        "internal/array.h",
        "internal/extension.h",
        "internal/map.h",
        "internal/map_entry.h",
        "internal/map_sorter.h",
        "internal/message.h",
        "map_gencode_util.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal_types",
        ":types",
        ":value",
        "//upb:base",
        "//upb:base_internal",
        "//upb:hash",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:port",
    ],
)

cc_library(
    name = "internal_types",
    hdrs = [
        "internal/types.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
    ],
)

cc_library(
    name = "message",
    hdrs = [
        "array.h",
        "map.h",
        "message.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        ":types",
        ":value",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:port",
    ],
)

cc_library(
    name = "promote",
    srcs = [
        "promote.c",
    ],
    hdrs = [
        "promote.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":accessors",
        ":accessors_internal",
        ":internal",
        ":message",
        ":tagged_ptr",
        ":types",
        "//upb:base",
        "//upb:eps_copy_input_stream",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:mini_table_internal",
        "//upb:port",
        "//upb:wire",
        "//upb:wire_internal",
        "//upb:wire_reader",
    ],
)

cc_library(
    name = "split64",
    hdrs = [
        "accessors_split64.h",
        "array_split64.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":accessors",
        ":message",
        "//upb:port",
    ],
)

cc_library(
    name = "tagged_ptr",
    hdrs = ["tagged_ptr.h"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":types",
        "//upb:port",
    ],
)

cc_library(
    name = "types",
    hdrs = [
        "types.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [],
)

cc_library(
    name = "value",
    hdrs = [
        "value.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":tagged_ptr",
        ":types",
        "//upb:base",
    ],
)

proto_library(
    name = "message_test_proto",
    testonly = 1,
    srcs = ["test.proto"],
    deps = ["//src/google/protobuf:test_messages_proto3_proto"],
)

upb_minitable_proto_library(
    name = "message_test_upb_minitable_proto",
    testonly = 1,
    deps = [":message_test_proto"],
)

upb_proto_library(
    name = "message_test_upb_proto",
    testonly = 1,
    deps = [":message_test_proto"],
)

upb_proto_reflection_library(
    name = "message_test_upb_proto_reflection",
    testonly = 1,
    deps = [":message_test_proto"],
)

cc_test(
    name = "accessors_test",
    srcs = ["accessors_test.cc"],
    deps = [
        ":accessors",
        ":message",
        "//:protobuf",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_descriptor",
        "//upb:mini_descriptor_internal",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_minitable",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)

cc_test(
    name = "array_test",
    srcs = ["array_test.cc"],
    deps = [
        ":message",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:mem",
    ],
)

cc_test(
    name = "copy_test",
    srcs = ["copy_test.cc"],
    deps = [
        ":accessors",
        ":copy",
        ":internal",
        ":message",
        "//:protobuf",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:wire",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)

cc_test(
    name = "map_test",
    srcs = ["map_test.cc"],
    deps = [
        ":message",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:mem",
    ],
)

cc_test(
    name = "promote_test",
    srcs = ["promote_test.cc"],
    deps = [
        ":accessors",
        ":copy",
        ":internal",
        ":message",
        ":promote",
        ":tagged_ptr",
        "//:protobuf",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_descriptor",
        "//upb:mini_descriptor_internal",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_proto_upb_minitable",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
    ],
)

# This test doesn't directly include any files from this subdir so it probably
# should live elsewhere.
cc_test(
    name = "test",
    srcs = ["test.cc"],
    deps = [
        ":message_test_upb_minitable_proto",
        ":message_test_upb_proto",
        ":message_test_upb_proto_reflection",
        ":value",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:json",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
        "//upb:reflection",
        "//upb:wire",
        "//upb/test:fuzz_util",
        "//upb/test:test_messages_proto3_upb_proto",
    ],
)

proto_library(
    name = "utf8_test_proto",
    testonly = 1,
    srcs = ["utf8_test.proto"],
    deps = ["//src/google/protobuf:test_messages_proto3_proto"],
)

upb_minitable_proto_library(
    name = "utf8_test_upb_minitable_proto",
    testonly = 1,
    deps = [":utf8_test_proto"],
)

upb_proto_library(
    name = "utf8_test_upb_proto",
    testonly = 1,
    deps = [":utf8_test_proto"],
)

cc_test(
    name = "utf8_test",
    srcs = ["utf8_test.cc"],
    deps = [
        ":utf8_test_upb_minitable_proto",
        ":utf8_test_upb_proto",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:mem",
        "//upb:wire",
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
