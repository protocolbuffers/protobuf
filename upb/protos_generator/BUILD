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

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

licenses(["notice"])

cc_binary(
    name = "protoc-gen-upb-protos",
    srcs = [
        "protoc-gen-upb-protos.cc",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":gen_utils",
        ":generator",
        ":names",
        ":output",
        "//upbc:file_layout",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//src/google/protobuf/compiler:code_generator",
    ],
)

cc_library(
    name = "generator",
    srcs = [
        "gen_accessors.cc",
        "gen_enums.cc",
        "gen_extensions.cc",
        "gen_messages.cc",
        "gen_repeated_fields.cc",
    ],
    hdrs = [
        "gen_accessors.h",
        "gen_enums.h",
        "gen_extensions.h",
        "gen_messages.h",
        "gen_repeated_fields.h",
    ],
    visibility = ["//visibility:private"],
    deps = [
        ":gen_utils",
        ":names",
        ":output",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "//upbc:common",
        "//upbc:file_layout",
        "//upbc:keywords",
        "//upbc:names",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "output",
    srcs = ["output.cc"],
    hdrs = ["output.h"],
    visibility = ["//visibility:private"],
    deps = [
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "gen_utils",
    srcs = ["gen_utils.cc"],
    hdrs = ["gen_utils.h"],
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//src/google/protobuf/compiler:code_generator",
    ],
)

cc_library(
    name = "names",
    srcs = ["names.cc"],
    hdrs = ["names.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":output",
        "//upbc:keywords",
    ],
)
