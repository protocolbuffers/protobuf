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
    name = "upb_proto_library_bzl",
    srcs = [
        "upb_c_proto_library.bzl",
        "upb_minitable_proto_library.bzl",
        "upb_proto_library.bzl",
        "upb_proto_reflection_library.bzl",
    ],
    visibility = ["//visibility:public"],
    deps = ["//bazel/private:upb_proto_library_internal_bzl"],
)
