# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "lex",
    srcs = [
        "atoi.c",
        "round_trip.c",
        "strtod.c",
        "unicode.c",
    ],
    hdrs = [
        "atoi.h",
        "round_trip.h",
        "strtod.h",
        "unicode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = ["//upb:port"],
)

cc_test(
    name = "atoi_test",
    srcs = ["atoi_test.cc"],
    deps = [
        ":lex",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "round_trip_test",
    srcs = ["round_trip_test.cc"],
    deps = [
        ":lex",
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
    visibility = ["//pkg:__pkg__"],
)
