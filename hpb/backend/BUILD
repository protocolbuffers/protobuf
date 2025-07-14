# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_library.bzl", "cc_library")

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "types",
    hdrs = ["types.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//hpb:multibackend",
    ] + select({
        "//hpb:hpb_backend_cpp": [
            "//src/google/protobuf:arena",
        ],
        "//hpb:hpb_backend_upb": [
            "//upb/mem",
        ],
    }),
)
