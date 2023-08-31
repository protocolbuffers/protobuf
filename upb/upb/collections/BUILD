# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

# begin:google_only
# load("//bazel:build_defs.bzl", "upb_kt_native_interop_hint")
#
# upb_kt_native_interop_hint(
#     name = "collections_aspect_hint",
#     strict_enums = ["upb_MapInsertStatus"],
# )
# end:google_only

cc_library(
    name = "collections",
    hdrs = [
        "array.h",
        "map.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//:base",
        "//:mem",
        "//:port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "array.c",
        "array.h",
        "map.c",
        "map.h",
        "map_sorter.c",
    ],
    hdrs = [
        "internal/array.h",
        "internal/map.h",
        "internal/map_entry.h",
        "internal/map_sorter.h",
        "map_gencode_util.h",
        "message_value.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//:base",
        "//:base_internal",
        "//:hash",
        "//:mem",
        "//:message_internal",
        "//:message_internal_types",
        "//:message_tagged_ptr",
        "//:mini_table",
        "//:port",
    ],
)

cc_library(
    name = "split64",
    hdrs = [
        "array_split64.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        "//:port",
    ],
)

cc_test(
    name = "array_test",
    srcs = ["array_test.cc"],
    deps = [
        ":collections",
        "//:base",
        "//:mem",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "map_test",
    srcs = ["map_test.cc"],
    deps = [
        ":collections",
        "//:base",
        "//:mem",
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
        "//cmake:__pkg__",
        "//python/dist:__pkg__",
    ]
)
# end:github_only
