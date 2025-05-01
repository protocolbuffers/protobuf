# Copyright (c) 2009-2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "field_parsers",
    srcs = [
        "cardinality.h",
        "dispatch.h",
        "field_fixed.c",
        "field_message.c",
        "field_string.c",
        "field_varint.c",
    ],
    hdrs = ["field_parsers.h"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__pkg__"],
    deps = [
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
        "//upb:port",
        "//upb/message:types",
        "//upb/wire:decoder",
        "//upb/wire:eps_copy_input_stream",
    ],
)

cc_library(
    name = "select",
    srcs = ["select.cc"],
    hdrs = ["select.h"],
    visibility = ["//upb_generator/minitable:__pkg__"],
    deps = [
        "//upb:base",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire_reader",
        "@abseil-cpp//absl/strings",
    ],
)
