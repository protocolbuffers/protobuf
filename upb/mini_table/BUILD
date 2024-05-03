# Copyright (c) 2009-2023, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)

cc_library(
    name = "compat",
    srcs = [
        "compat.c",
    ],
    hdrs = [
        "compat.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":mini_table",
        "//upb:base",
        "//upb:hash",
        "//upb:mem",
        "//upb:port",
    ],
)

cc_library(
    name = "mini_table",
    srcs = [
        "extension_registry.c",
        "message.c",
    ],
    hdrs = [
        "enum.h",
        "extension.h",
        "extension_registry.h",
        "field.h",
        "file.h",
        "message.h",
        "sub.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb:base",
        "//upb:hash",
        "//upb:mem",
        "//upb:message_types",
        "//upb:port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "internal/message.c",
    ],
    hdrs = [
        "internal/enum.h",
        "internal/extension.h",
        "internal/field.h",
        "internal/file.h",
        "internal/message.h",
        "internal/sub.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//upb:base",
        "//upb:message_types",
        "//upb:port",
    ],
)

cc_test(
    name = "compat_test",
    srcs = ["compat_test.cc"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "//upb:mini_table_compat",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto3_upb_minitable",
        "//upb/test:test_upb_proto",
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
