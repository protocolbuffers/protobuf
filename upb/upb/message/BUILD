# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")
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
        "//:base",
        "//:collections",
        "//:collections_internal",
        "//:eps_copy_input_stream",
        "//:mini_table",
        "//:mini_table_internal",
        "//:port",
        "//:wire",
        "//:wire_reader",
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
        "//:collections_internal",
        "//:mini_table_internal",
        "//:port",
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
        "//:base",
        "//:collections_internal",
        "//:mem",
        "//:mini_table",
        "//:mini_table_internal",
        "//:port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "message.c",
        "message.h",
    ],
    hdrs = [
        "internal/extension.h",
        "internal/message.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal_types",
        ":types",
        "//:base",
        "//:base_internal",
        "//:hash",
        "//:mem",
        "//:mini_table",
        "//:port",
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
        "message.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        ":types",
        "//:base",
        "//:mem",
        "//:mini_table",
        "//:port",
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
        ":internal",
        ":message",
        ":types",
        "//:base",
        "//:collections",
        "//:collections_internal",
        "//:eps_copy_input_stream",
        "//:hash",
        "//:mem",
        "//:mini_table",
        "//:port",
        "//:wire",
        "//:wire_reader",
    ],
)

cc_library(
    name = "split64",
    hdrs = [
        "accessors_split64.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":accessors",
        "//:port",
    ],
)

cc_library(
    name = "tagged_ptr",
    hdrs = ["tagged_ptr.h"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":types",
        "//:port",
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

# TODO(salo): Move these proto library targets to //third_party/protobuf/BUILD
# after we have the monorepo.
proto_library(
    name = "message_test_proto",
    testonly = 1,
    srcs = ["test.proto"],
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto3_proto"],
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
        "//:base",
        "//:collections",
        "//:mini_descriptor",
        "//:mini_descriptor_internal",
        "//:mini_table",
        "//:port",
        "//:wire",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
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
        "//:base",
        "//:collections",
        "//:mem",
        "//:mini_table",
        "//:wire",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "promote_test",
    srcs = ["promote_test.cc"],
    deps = [
        ":accessors",
        ":copy",
        ":promote",
        "//:base",
        "//:collections",
        "//:mem",
        "//:mini_descriptor_internal",
        "//:mini_table",
        "//:port",
        "//:wire",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

# This test doesn't directly include any files from this subdir so it probably
# should live elsewhere.
cc_test(
    name = "test",
    srcs = ["test.cc"],
    deps = [
        ":message_test_upb_proto",
        ":message_test_upb_proto_reflection",
        "//:base",
        "//:json",
        "//:mem",
        "//:reflection",
        "//:wire",
        "//upb/test:fuzz_util",
        "//upb/test:test_messages_proto3_upb_proto",
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
        "//cmake:__pkg__",
        "//python/dist:__pkg__",
    ]
)
# end:github_only
