# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//upb/bazel:copts.bzl", "UPB_DEFAULT_COPTS")

package(default_applicable_licenses = ["//:license"])

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
        "//upb/base",
        "//upb/base:internal",
        "//upb/mem",
        "//upb/port",
    ],
)

cc_test(
    name = "test",
    srcs = ["test.cc"],
    deps = [
        ":hash",
        "//upb/mem",
        "//upb/port",
        "@abseil-cpp//absl/container:flat_hash_map",
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
