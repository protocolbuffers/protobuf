# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

cc_library(
    name = "base",
    srcs = [
        "status.c",
    ],
    hdrs = [
        "descriptor_constants.h",
        "status.h",
        "status.hpp",
        "string_view.h",
        "upcast.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:port",
    ],
)

cc_library(
    name = "internal",
    hdrs = [
        "internal/endian.h",
        "internal/log2.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = ["//upb:port"],
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
        "//python/dist:__pkg__",
        "//upb/cmake:__pkg__",
    ],
)
# end:github_only
