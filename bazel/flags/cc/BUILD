# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")
load("//bazel/private:compat_flag.bzl", "compat_label_flag", "compat_string_list_flag")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

exports_files(["BUILD"])

compat_label_flag(
    name = "proto_toolchain_for_cc",
    build_setting_default = "@bazel_tools//tools/proto:cc_toolchain",
    fragment_field = "proto_toolchain_for_cc",
)

alias(
    name = "protocopt",
    actual = "//bazel/flags:protocopt",
    deprecation = "Use //bazel/flags:protocopt instead.",
)

compat_string_list_flag(
    name = "cc_proto_library_header_suffixes",
    build_setting_default = [".pb.h"],
    fragment_field = "cc_proto_library_header_suffixes",
    scope = "universal",
)
