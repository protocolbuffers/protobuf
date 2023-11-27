# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

cc_library(
    name = "mem",
    hdrs = [
        "alloc.h",
        "arena.h",
        "arena.hpp",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb:port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "alloc.c",
        "alloc.h",
        "arena.c",
        "arena.h",
    ],
    hdrs = [
        "internal/arena.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__pkg__"],
    deps = [
        "//upb:port",
    ],
)

cc_test(
    name = "arena_test",
    srcs = ["arena_test.cc"],
    deps = [
        "@com_google_googletest//:gtest_main",
        "//upb:mem",
        "//upb:mem_internal",
        "//upb:port",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/random:distributions",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
    ],
)

# begin:github_only
filegroup(
    name = "source_files",
    srcs = glob(
        [
            "**/*.c",
            "**/*.h",
            "**/*.hpp",
        ],
    ),
    visibility = [
        "//upb/cmake:__pkg__",
        "//python/dist:__pkg__",
    ]
)
# end:github_only
