# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

licenses(["notice"])

bzl_library(
    name = "upb_cc_proto_library_bzl",
    srcs = ["upb_cc_proto_library.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "@bazel_skylib//lib:paths",
        "//bazel:upb_proto_library_bzl",
        "@bazel_tools//tools/cpp:toolchain_utils.bzl",
    ],
)
