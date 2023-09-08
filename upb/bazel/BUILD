# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_python//python:defs.bzl", "py_binary")
load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

licenses(["notice"])

exports_files(
    ["workspace_deps.bzl"],
    visibility = ["//cmake:__pkg__"],
)

py_binary(
    name = "amalgamate",
    srcs = ["amalgamate.py"],
    visibility = ["//:__pkg__"],
)

# py_proto_library() is private rule, only intended for internal use by upb.
# Hopefully py_proto_library() will eventually be availble in rules_proto or
# another upstream package.
bzl_library(
    name = "py_proto_library_bzl",
    srcs = ["py_proto_library.bzl"],
)

bzl_library(
    name = "upb_proto_library_bzl",
    srcs = ["upb_proto_library.bzl"],
    visibility = ["//visibility:public"],
    deps = [
        "@bazel_skylib//lib:paths",
        "@bazel_tools//tools/cpp:toolchain_utils.bzl",
        "@rules_proto//proto:defs",
    ],
)
