# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//lib:selects.bzl", "selects")
load("@rules_python//python:defs.bzl", "py_binary")

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

licenses(["notice"])

# begin:google_only
# selects.config_setting_group(
#     name = "android_opt",
#     match_all = [
#         "//tools/cc_target_os:android",
#         "//tools/compilation_mode:opt",
#     ],
# )
# end:google_only

py_binary(
    name = "amalgamate",
    srcs = ["amalgamate.py"],
    visibility = ["//upb:__pkg__"],
)
