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
        "@com_google_absl//absl/strings",
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
