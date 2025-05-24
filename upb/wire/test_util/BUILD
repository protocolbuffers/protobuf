# Copyright (c) 2025, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "wire_message",
    testonly = True,
    srcs = ["wire_message.cc"],
    hdrs = ["wire_message.h"],
    visibility = ["//upb:__subpackages__"],
    deps = [
        "//upb:wire_reader",
        "//upb/base:internal",
        "@abseil-cpp//absl/log:absl_check",
    ],
)
