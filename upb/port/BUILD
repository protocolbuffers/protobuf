# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//upb/bazel:copts.bzl", "UPB_DEFAULT_COPTS", "UPB_DEFAULT_FEATURES")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "port",
    srcs = ["port.c"],
    hdrs = [
        "atomic.h",
        "sanitizers.h",
        "vsnprintf_compat.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    defines = select({
        "//upb:fasttable_enabled_setting": ["UPB_ENABLE_FASTTABLE"],
        "//conditions:default": [],
    }) + select({
        "//upb:android": [],
        "@platforms//os:linux": [],
        "//conditions:default": ["UPB_NO_LINKTIME_SORT"],
    }),
    features = UPB_DEFAULT_FEATURES,
    textual_hdrs = [
        # We must list the headers explicitly here instead of relying on the
        # :inc filegroup below. gRPC auto-generates various build configs from
        # the Bazel graph, and this logic does not know how to handle
        # filegroups. We might be able to replace these headers with just
        # ":inc" after gRPC starts using upb's CMake build.
        "def.inc",
        "undef.inc",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "inc",
    srcs = [
        "def.inc",
        "undef.inc",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "port_test",
    srcs = ["port_test.cc"],
    deps = [
        ":port",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

filegroup(
    name = "source_files",
    srcs = glob(
        [
            "**/*.h",
            "**/*.inc",
        ],
    ),
    visibility = [
        "//python/dist:__pkg__",
        "//upb/cmake:__pkg__",
    ],
)
