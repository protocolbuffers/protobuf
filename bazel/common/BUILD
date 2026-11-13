# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(default_applicable_licenses = ["//:license"])

bzl_library(
    name = "proto_common_bzl",
    srcs = [
        "proto_common.bzl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":proto_lang_toolchain_info_bzl",
        "//bazel/private:toolchain_helpers_bzl",
        "@proto_bazel_features//:features",
    ],
)

bzl_library(
    name = "proto_info_bzl",
    srcs = [
        "proto_info.bzl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/private:proto_info_bzl",
    ],
)

bzl_library(
    name = "proto_lang_toolchain_info_bzl",
    srcs = [
        "proto_lang_toolchain_info.bzl",
    ],
    visibility = ["//visibility:public"],
)

filegroup(
    name = "for_bazel_tests",
    testonly = True,
    srcs = [
        "BUILD",
        "proto_common_bzl",
        "proto_info_bzl",
        "proto_lang_toolchain_info_bzl",
    ],
    visibility = ["//bazel:__pkg__"],
)
