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
    name = "template_help",
    hdrs = ["template_help.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//hpb:ptr",
    ],
)

cc_library(
    name = "internal",
    hdrs = ["internal.h"],
    visibility = [
        "//hpb:__subpackages__",
    ],
    deps = [
        "//upb:mem",
        "//upb:message",
    ],
)

cc_library(
    name = "message_lock",
    srcs = ["message_lock.cc"],
    hdrs = [
        "message_lock.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//upb:friends"],
    deps = [
        "//upb:mem",
        "//upb:message",
        "//upb:message_copy",
        "//upb:message_promote",
        "//upb:mini_table",
        "//upb:wire",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/synchronization",
    ],
)

cc_test(
    name = "message_lock_test",
    srcs = ["message_lock_test.cc"],
    deps = [
        "//hpb",
        "//hpb/internal:message_lock",
        "//src/google/protobuf/compiler/hpb/tests:test_model_hpb_proto",
        "//upb:mem",
        "@com_google_absl//absl/hash",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
