# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "types",
    hdrs = ["types.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//hpb:multibackend",
    ] + select({
        "//hpb:hpb_backend_cpp": [
            "//hpb/backend/cpp:error",
            "//src/google/protobuf:arena",
        ],
        "//hpb:hpb_backend_upb": [
            "//hpb/backend/upb:error",
            "//upb/mem",
        ],
    }),
)

cc_test(
    name = "shared_test",
    srcs = ["shared_test.cc"],
    deps = [
        "//hpb",
        "//hpb:arena",
        "//hpb:ptr",
        "//hpb_generator/tests:test_model_hpb_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
