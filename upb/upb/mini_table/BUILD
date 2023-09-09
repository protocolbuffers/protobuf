# Copyright (c) 2009-2023, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)

# begin:google_only
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
