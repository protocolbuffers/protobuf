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
#     name = "wire_aspect_hint",
#     strict_enums = [
#         "upb_DecodeStatus",
#         "upb_EncodeStatus",
#         "upb_WireType",
#     ],
# )
# end:google_only

cc_library(
    name = "wire",
    srcs = [
    ],
    hdrs = [
        "decode.h",
        "encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        ":types",
        "//:mem",
        "//:message",
        "//:mini_table",
        "//:port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "decode.c",
        "decode.h",
        "decode_fast.c",
        "encode.c",
        "encode.h",
    ],
    hdrs = [
        "decode_fast.h",
        "internal/common.h",
        "internal/decode.h",
        "internal/swap.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":reader",
        ":types",
        "//:base",
        "//:collections_internal",
        "//:mem",
        "//:mem_internal",
        "//:message",
        "//:message_accessors_internal",
        "//:message_internal",
        "//:message_internal_types",
        "//:mini_table",
        "//:port",
        "@utf8_range",
    ],
)

cc_library(
    name = "reader",
    srcs = [
        "internal/swap.h",
        "reader.c",
    ],
    hdrs = ["reader.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":types",
        "//:port",
    ],
)

cc_library(
    name = "types",
    hdrs = ["types.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "eps_copy_input_stream",
    srcs = ["eps_copy_input_stream.c"],
    hdrs = ["eps_copy_input_stream.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//:mem",
        "//:port",
    ],
)

cc_test(
    name = "eps_copy_input_stream_test",
    srcs = ["eps_copy_input_stream_test.cc"],
    deps = [
        ":eps_copy_input_stream",
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
