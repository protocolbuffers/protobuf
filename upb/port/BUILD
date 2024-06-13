# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

cc_library(
    name = "port",
    hdrs = [
        "atomic.h",
        "vsnprintf_compat.h",
    ],
    copts = UPB_DEFAULT_COPTS,
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

# begin:github_only
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
# end:github_only
