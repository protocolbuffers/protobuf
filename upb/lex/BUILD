# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

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
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/strings",
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
