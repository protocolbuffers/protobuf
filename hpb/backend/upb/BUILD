# Copyright (c) 2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "upb",
    hdrs = ["upb.h"],
    visibility = ["//upb:friends"],
    deps = [
        ":interop",
        "//hpb:arena",
        "//hpb:ptr",
        "//hpb/internal",
        "//hpb/internal:message_lock",
        "//hpb/internal:template_help",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "interop",
    hdrs = ["interop.h"],
    visibility = [
        "//hpb:__subpackages__",
        "//src/google/protobuf/compiler:__subpackages__",
    ],
    deps = [
        "//hpb:ptr",
        "//hpb/internal",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/mini_table",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_test(
    name = "interop_test",
    srcs = ["interop_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":interop",
        "//src/google/protobuf/compiler/hpb/tests:test_model_hpb_proto",
        "//src/google/protobuf/compiler/hpb/tests:test_model_upb_proto",
        "//upb/mem",
        "//upb/message",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "types",
    hdrs = ["types.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//upb/mem",
    ],
)
