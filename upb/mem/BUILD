# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)

config_setting(
    name = "opt_build",
    values = {
        "compilation_mode": "opt",
    },
)

cc_library(
    name = "mem",
    srcs = [
        "alloc.c",
        "arena.c",
    ],
    hdrs = [
        "alloc.h",
        "arena.h",
        "arena.hpp",
    ],
    copts = UPB_DEFAULT_COPTS + select({
        ":opt_build": [],
        "//conditions:default": ["-DUPB_DEBUG_REFS"],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//upb:port",
    ] + select({
        ":opt_build": [],
        "//conditions:default": [":debug"],
    }),
)

cc_library(
    name = "internal",
    hdrs = [
        "internal/arena.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:port",
    ],
)

cc_library(
    name = "debug",
    srcs = [
        "debug/ref.cc",
    ],
    hdrs = [
        "debug/ref.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:port",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "arena_test",
    srcs = ["arena_test.cc"],
    deps = [
        ":mem",
        "//upb:port",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/random:distributions",
        "@com_google_absl//absl/synchronization",
        "@com_google_absl//absl/time",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "ref_test",
    srcs = ["debug/ref_test.cc"],
    deps = [
        ":debug",
        ":mem",
        "//upb:port",
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
            "**/*.hpp",
            "**/ref.cc",
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
