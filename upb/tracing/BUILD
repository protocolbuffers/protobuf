# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS", "UPB_DEFAULT_CPPOPTS")
load(
    "//bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

cc_library(
    name = "tracing",
    srcs = [
        "tracing.c",
    ],
    hdrs = [
        "tracing.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:port",
    ],
)

# begin:google_only
#
# upb_minitable_proto_library(
#     name = "trace_tracing_proto",
#     testonly = 1,
#     deps = ["//upb/test:test_proto"],
# )
#
# cc_test(
#     name = "test_tracing",
#     srcs = ["test_tracing.cc"],
#     copts = UPB_DEFAULT_CPPOPTS + ["-DUPB_TRACING_ENABLED"],
#     deps = [
#         ":trace_tracing_proto",
#         "@com_google_googletest//:gtest", "@com_google_googletest//:gtest_main",
#         "//upb:mini_table",
#     ],
# )
# end:google_only

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
