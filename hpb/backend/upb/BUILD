# Copyright (c) 2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "upb",
    hdrs = ["upb.h"],
    visibility = ["//upb:friends"],
    deps = [
        ":interop",
        "//hpb:ptr",
        "//hpb/internal",
        "//hpb/internal:template_help",
        "//upb:mini_table",
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
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
    ],
)
