# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

cc_library(
    name = "collections",
    hdrs = [
        "array.h",
        "map.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb:base",
        "//upb:mem",
        "//upb:message_value",
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
    ],
    hdrs = [
        "internal/array.h",
        "internal/map.h",
        "internal/map_entry.h",
        "internal/map_sorter.h",
        "map_gencode_util.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:base",
        "//upb:base_internal",
        "//upb:hash",
        "//upb:mem",
        "//upb:message_internal",
        "//upb:message_internal_types",
        "//upb:message_types",
        "//upb:message_value",
        "//upb:port",
    ],
)

cc_library(
    name = "split64",
    hdrs = [
        "array_split64.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        "//upb:port",
    ],
)

cc_test(
    name = "array_test",
    srcs = ["array_test.cc"],
    deps = [
        ":collections",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
        "//upb:mem",
    ],
)

cc_test(
    name = "map_test",
    srcs = ["map_test.cc"],
    deps = [
        ":collections",
        "@com_google_googletest//:gtest_main",
        "//upb:base",
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
