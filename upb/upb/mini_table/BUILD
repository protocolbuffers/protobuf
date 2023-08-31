# Copyright (c) 2009-2023, Google LLC
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
# cc_library(
#     name = "compat",
#     srcs = [
#         "compat.c",
#     ],
#     hdrs = [
#         "compat.h",
#     ],
#     compatible_with = ["//buildenv/target:non_prod"],
#     copts = UPB_DEFAULT_COPTS,
#     visibility = ["//:__pkg__"],
#     deps = [
#         ":mini_table",
#         "//:base",
#         "//:port",
#     ],
# )
#
# upb_kt_native_interop_hint(name = "mini_table_aspect_hint")
# end:google_only

cc_library(
    name = "mini_table",
    srcs = [
        "extension_registry.c",
        "message.c",
    ],
    hdrs = [
        "enum.h",
        "extension.h",
        "extension_registry.h",
        "field.h",
        "file.h",
        "message.h",
        "sub.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":internal",
        "//:base",
        "//:hash",
        "//:mem",
        "//:message_types",
        "//:port",
    ],
)

cc_library(
    name = "internal",
    srcs = [
        "internal/message.c",
    ],
    hdrs = [
        "internal/enum.h",
        "internal/extension.h",
        "internal/field.h",
        "internal/file.h",
        "internal/message.h",
        "internal/sub.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//:base",
        "//:message_types",
        "//:port",
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
