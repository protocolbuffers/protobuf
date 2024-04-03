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
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

cc_library(
    name = "message",
    srcs = [
        "accessors.c",
        "array.c",
        "compat.c",
        "map.c",
        "map_sorter.c",
        "message.c",
    ],
    hdrs = [
        "accessors.h",
        "array.h",
        "compat.h",
        "map.h",
        "map_gencode_util.h",
        "message.h",
        "tagged_ptr.h",
        "value.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        ":types",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:port",
        "//upb/base:internal",
        "//upb/hash",
        "//upb/mini_table:internal",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "internal/compare_unknown.c",
        "internal/extension.c",
        "internal/message.c",
        "value.h",
    ],
    hdrs = [
        "internal/accessors.h",
        "internal/array.h",
        "internal/compare_unknown.h",
        "internal/extension.h",
        "internal/map.h",
        "internal/map_sorter.h",
        "internal/message.h",
        "internal/tagged_ptr.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":types",
        "//upb:base",
        "//upb:eps_copy_input_stream",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire_reader",
        "//upb/base:internal",
        "//upb/hash",
        "//upb/mini_table:internal",
    ],
)

cc_library(
    name = "compare",
    srcs = [
        "compare.c",
    ],
    hdrs = [
        "compare.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        ":message",
        "//upb:base",
        "//upb:mini_table",
        "//upb:port",
        "//upb/mini_table:internal",
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
        ":internal",
        ":message",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:port",
        "//upb/base:internal",
        "//upb/mini_table:internal",
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
        ":internal",
        ":message",
        "//upb:base",
        "//upb:eps_copy_input_stream",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire",
        "//upb:wire_reader",
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
        ":message",
        "//upb:port",
    ],
)

cc_library(
    name = "types",
    hdrs = [
        "internal/map_entry.h",
        "internal/types.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:base",
        "//upb:port",
        "//upb/hash",
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

upb_c_proto_library(
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
        ":message",
        "//:protobuf",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_descriptor",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire",
        "//upb/mini_descriptor:internal",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_minitable",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "array_test",
    srcs = ["array_test.cc"],
    deps = [
        ":message",
        "//upb:base",
        "//upb:mem",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "compare_unknown_test",
    srcs = ["internal/compare_unknown_test.cc"],
    deps = [
        ":internal",
        "//upb:port",
        "//upb:wire_reader",
        "//upb/base:internal",
        "@com_google_absl//absl/types:variant",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "copy_test",
    srcs = ["copy_test.cc"],
    deps = [
        ":copy",
        ":internal",
        ":message",
        "//:protobuf",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "map_test",
    srcs = ["map_test.cc"],
    deps = [
        ":message",
        "//upb:base",
        "//upb:mem",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "promote_test",
    srcs = ["promote_test.cc"],
    deps = [
        ":copy",
        ":internal",
        ":message",
        ":promote",
        "//:protobuf",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_descriptor",
        "//upb:mini_table",
        "//upb:wire",
        "//upb/mini_descriptor:internal",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_proto_upb_minitable",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# This test doesn't directly include any files from this subdir so it probably
# should live elsewhere.
cc_test(
    name = "test",
    srcs = ["test.cc"],
    deps = [
        ":internal",
        ":message",
        ":message_test_upb_minitable_proto",
        ":message_test_upb_proto",
        ":message_test_upb_proto_reflection",
        "//upb:base",
        "//upb:json",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:reflection",
        "//upb:wire",
        "//upb/test:fuzz_util",
        "//upb/test:test_messages_proto3_upb_proto",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "utf8_test_proto",
    testonly = 1,
    srcs = ["utf8_test.proto"],
)

proto_library(
    name = "utf8_test_proto2_proto",
    testonly = 1,
    srcs = ["utf8_test_proto2.proto"],
)

upb_minitable_proto_library(
    name = "utf8_test_upb_minitable_proto",
    testonly = 1,
    deps = [":utf8_test_proto"],
)

upb_minitable_proto_library(
    name = "utf8_test_proto2_upb_minitable_proto",
    testonly = 1,
    deps = [":utf8_test_proto2_proto"],
)

upb_c_proto_library(
    name = "utf8_test_upb_proto",
    testonly = 1,
    deps = [":utf8_test_proto"],
)

upb_c_proto_library(
    name = "utf8_test_proto2_upb_proto",
    testonly = 1,
    deps = [":utf8_test_proto2_proto"],
)

cc_test(
    name = "utf8_test",
    srcs = ["utf8_test.cc"],
    deps = [
        ":utf8_test_proto2_upb_minitable_proto",
        ":utf8_test_proto2_upb_proto",
        ":utf8_test_upb_minitable_proto",
        ":utf8_test_upb_proto",
        "//upb:base",
        "//upb:mem",
        "//upb:wire",
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

# begin:github_only
filegroup(
    name = "test_srcs",
    srcs = glob(
        [
            "**/*test.cc",
        ],
        exclude = ["promote_test.cc"],
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
# end:github_only
