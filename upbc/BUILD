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
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

licenses(["notice"])

proto_library(
    name = "code_generator_request",
    srcs = ["code_generator_request.proto"],
    visibility = ["//:friends"],
    deps = ["@com_google_protobuf//:compiler_plugin_proto"],
)

upb_proto_library(
    name = "code_generator_request_upb_proto",
    visibility = ["//:friends"],
    deps = [":code_generator_request"],
)

upb_proto_reflection_library(
    name = "code_generator_request_upb_proto_reflection",
    visibility = ["//:friends"],
    deps = [":code_generator_request"],
)

upb_proto_library(
    name = "plugin_upb_proto",
    visibility = ["//:friends"],
    deps = ["@com_google_protobuf//:compiler_plugin_proto"],
)

upb_proto_reflection_library(
    name = "plugin_upb_proto_reflection",
    visibility = ["//:friends"],
    deps = ["@com_google_protobuf//:compiler_plugin_proto"],
)

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_binary(
    name = "protoc-gen-upb",
    srcs = ["protoc-gen-upb.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        "//:mini_table",
        "//:port",
        "//:upb",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_binary(
    name = "protoc-gen-upbdefs",
    srcs = [
        "protoc-gen-upbdefs.cc",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_binary(
    name = "protoc-gen-upbdev",
    srcs = [
        "code_generator_request.c",
        "code_generator_request.h",
        "protoc-gen-upbdev.cc",
        "subprocess.cc",
        "subprocess.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":code_generator_request_upb_proto",
        ":code_generator_request_upb_proto_reflection",
        ":plugin_upb_proto",
        ":plugin_upb_proto_reflection",
        "//:json",
        "//:mini_descriptor",
        "//:mini_table",
        "//:port",
        "//:reflection",
        "//:upb",
        "@com_google_absl//absl/strings",
    ],
)
