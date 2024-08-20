# Copyright (c) 2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

# begin:google_only
# package(default_applicable_licenses = ["//src/google/protobuf:license"])
# end:google_only

cc_library(
    name = "upb",
    hdrs = ["upb.h"],
    compatible_with = ["//buildenv/target:non_prod"],
    visibility = ["//upb:friends"],
    deps = [
        "//hpb:ptr",
        "//upb:mini_table",
    ],
)
