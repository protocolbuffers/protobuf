# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

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
        "//upb:mem",
        "//upb:port",
        "//upb/base:internal",
    ],
)

cc_test(
    name = "test",
    srcs = ["test.cc"],
    deps = [
        ":hash",
        "//upb:mem",
        "//upb:port",
        "@com_google_absl//absl/container:flat_hash_map",
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
    visibility = ["//pkg:__pkg__"],
)
# end:github_only
