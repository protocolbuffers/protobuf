# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

bzl_library(
    name = "hpb_proto_library_bzl",
    srcs = ["hpb_proto_library.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel:upb_proto_library_bzl",
        "@bazel_skylib//lib:paths",
        "@bazel_tools//tools/cpp:toolchain_utils.bzl",
    ],
)
