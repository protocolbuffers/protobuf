# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "cpp",
    hdrs = ["cpp.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//hpb:arena",
        "//hpb:ptr",
        "//hpb/internal:template_help",
        "//src/google/protobuf:protobuf_lite",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "types",
    hdrs = ["types.h"],
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//src/google/protobuf:arena",
    ],
)
