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
    name = "upb_proto_library_bzl",
    srcs = [
        "upb_c_proto_library.bzl",
        "upb_minitable_proto_library.bzl",
        "upb_proto_library.bzl",
        "upb_proto_reflection_library.bzl",
        "visibility.bzl",
    ],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//bazel/common:proto_common_bzl",
        "//bazel/common:proto_info_bzl",
        "//upb/bazel/private:upb_proto_library_internal_bzl",
        "@rules_cc//cc/common",
    ],
)
