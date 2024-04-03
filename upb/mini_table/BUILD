# Copyright (c) 2009-2023, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
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
        "//upb:mem",
        "//upb:port",
        "//upb/hash",
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
        "internal/size_log2.h",
        "internal/sub.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:base",
        "//upb:mem",
        "//upb:port",
        "//upb/hash",
        "//upb/message:types",
    ],
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
        "//upb:mem",
        "//upb:port",
        "//upb/hash",
    ],
)

cc_test(
    name = "compat_test",
    srcs = ["compat_test.cc"],
    deps = [
        "//upb:mini_table_compat",
        "//upb/test:test_messages_proto2_upb_minitable",
        "//upb/test:test_messages_proto3_upb_minitable",
        "//upb/test:test_upb_proto",
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
    ),
    visibility = ["//upb:__pkg__"],
)
# end:github_only
