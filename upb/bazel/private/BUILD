# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@bazel_skylib//lib:selects.bzl", "selects")

bzl_library(
    name = "upb_proto_library_internal_bzl",
    srcs = [
        "upb_proto_library_internal/aspect.bzl",
        "upb_proto_library_internal/cc_library_func.bzl",
        "upb_proto_library_internal/copts.bzl",
        "upb_proto_library_internal/rule.bzl",
    ],
    visibility = [
        "//upb/bazel:__pkg__",
    ],
    deps = [
        "//bazel/common:proto_common_bzl",
        "//bazel/common:proto_info_bzl",
        "@bazel_skylib//lib:paths",
        "@bazel_tools//tools/cpp:toolchain_utils.bzl",
    ],
)
