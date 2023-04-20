# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)
load(
    "//protos/bazel:upb_cc_proto_library.bzl",
    "upb_cc_proto_library",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
)
load(
    "@rules_cc//cc:defs.bzl",
    "cc_proto_library",
)

licenses(["notice"])

proto_library(
    name = "test_model_proto",
    srcs = [
        "child_model.proto",
        "test_enum.proto",
        "test_extension.proto",
        "test_model.proto",
    ],
)

proto_library(
    name = "no_package_proto",
    srcs = [
        "no_package.proto",
    ],
)

proto_library(
    name = "naming_conflict_proto",
    srcs = [
        "naming_conflict.proto",
    ],
)

proto_library(
    name = "no_package_enum_user_proto",
    srcs = [
        "no_package_enum_user.proto",
    ],
    deps = [":no_package_proto"],
)

upb_proto_library(
    name = "test_model_upb_proto",
    visibility = [
        "//protos:__pkg__",
    ],
    deps = [":test_model_proto"],
)

upb_cc_proto_library(
    name = "test_model_upb_cc_proto",
    visibility = ["//protos:__pkg__"],
    deps = [":test_model_proto"],
)

upb_cc_proto_library(
    name = "naming_conflict_upb_cc_proto",
    visibility = ["//protos:__pkg__"],
    deps = [":naming_conflict_proto"],
)

upb_cc_proto_library(
    name = "no_package_upb_cc_proto",
    deps = [
        ":no_package_proto",
    ],
)

upb_cc_proto_library(
    name = "no_package_enum_user_upb_cc_proto",
    deps = [
        ":no_package_enum_user_proto",
    ],
)

cc_proto_library(
    name = "test_model_cc_proto",
    deps = [":test_model_proto"],
)

# begin:google_only
# proto_library(
#     name = "legacy_name_proto",
#     srcs = [
#         "legacy-name.proto",
#     ],
# )
#
# upb_cc_proto_library(
#     name = "legacy_name_test_proto",
#     visibility = [
#         "//visibility:private",  # Only private by automation, not intent. Owner may accept CLs adding visibility. See go/scheuklappen#explicit-private.
#     ],
#     deps = [":legacy_name_proto"],
# )
# end:google_only

cc_test(
    name = "test_generated_cc_code",
    srcs = ["test_generated.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        # begin:google_only
#         ":legacy_name_test_proto",
        # end:google_only
        ":no_package_upb_cc_proto",
        ":test_model_upb_cc_proto",
        ":test_model_upb_proto",
        ":naming_conflict_upb_cc_proto",
        "@com_google_googletest//:gtest_main",
        "//:upb",
        "//protos",
    ],
)
