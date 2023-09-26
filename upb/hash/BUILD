# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

cc_library(
    name = "hash",
    srcs = [
        "common.c",
    ],
    hdrs = [
        "common.h",
        "int_table.h",
        "str_table.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:base",
        "//upb:base_internal",
        "//upb:mem",
        "//upb:port",
    ],
)

cc_test(
    name = "test",
    srcs = ["test.cc"],
    deps = [
        ":hash",
        "@com_google_googletest//:gtest_main",
        "//upb:mem",
        "//upb:port",
        "@com_google_absl//absl/container:flat_hash_map",
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
