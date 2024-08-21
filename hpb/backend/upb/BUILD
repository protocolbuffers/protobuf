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
    compatible_with = ["//buildenv/target:non_prod"],
    visibility = [
        "//hpb:__subpackages__",
        "//video/youtube/utils/elements/client/component_type:__pkg__",
        "//video/youtube/utils/elements/idom/styling/style_provider:__pkg__",
        "//video/youtube/utils/elements/idom/styling/theme_provider:__pkg__",
        "//video/youtube/utils/elements/javascript/client/proto/upb/native:__pkg__",
        "//video/youtube/utils/elements/upb/crosslang:__pkg__",
    ],
    deps = [
        "//hpb:ptr",
        "//upb:mini_table",
    ],
)
