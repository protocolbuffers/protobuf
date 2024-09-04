# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//bazel:upb_minitable_proto_library.bzl",
    "upb_minitable_proto_library",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
    "upb_proto_reflection_library",
)
load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)
load(
    "//upb_generator:bootstrap_compiler.bzl",
    "bootstrap_cc_library",
    "bootstrap_upb_proto_library",
)

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

proto_library(
    name = "code_generator_request",
    srcs = ["code_generator_request.proto"],
    visibility = ["//upb:friends"],
    deps = ["//:compiler_plugin_proto"],
)

upb_c_proto_library(
    name = "code_generator_request_upb_proto",
    visibility = ["//upb:friends"],
    deps = [":code_generator_request"],
)

upb_proto_reflection_library(
    name = "code_generator_request_upb_proto_reflection",
    visibility = ["//upb:friends"],
    deps = [":code_generator_request"],
)

upb_minitable_proto_library(
    name = "code_generator_request_upb_minitable_proto",
    visibility = ["//upb:friends"],
    deps = [":code_generator_request"],
)

bootstrap_upb_proto_library(
    name = "plugin_upb_proto",
    bootstrap_hdr = "plugin_bootstrap.h",
    google3_src_files = [
        "net/proto2/compiler/proto/profile.proto",
        "third_party/protobuf/compiler/plugin.proto",
    ],
    google3_src_rules = [
        "//net/proto2/proto:descriptor_proto_source",
        "//net/proto2/compiler/proto:profile.proto",
        "//src/google/protobuf/compiler:plugin_proto_source",
    ],
    oss_src_files = ["google/protobuf/compiler/plugin.proto"],
    oss_src_rules = [
        "//:descriptor_proto_srcs",
        "//src/google/protobuf/compiler:plugin_proto_src",
    ],
    oss_strip_prefix = "third_party/protobuf/github/bootstrap/src",
    proto_lib_deps = ["//:compiler_plugin_proto"],
    deps = ["//upb/reflection:descriptor_upb_proto"],
)

upb_proto_reflection_library(
    name = "plugin_upb_proto_reflection",
    visibility = ["//upb:friends"],
    deps = ["//:compiler_plugin_proto"],
)

bootstrap_cc_library(
    name = "common",
    srcs = [
        "common.cc",
    ],
    hdrs = [
        "common.h",
    ],
    bootstrap_deps = [
        "//upb/reflection:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//upb:friend_generators"],
    deps = [
        ":mangle",
        "//src/google/protobuf:port",
        "//upb:mini_table",
        "//upb:port",
        "@com_google_absl//absl/strings",
    ],
)

bootstrap_cc_library(
    name = "file_layout",
    srcs = [
        "file_layout.cc",
    ],
    hdrs = [
        "file_layout.h",
    ],
    bootstrap_deps = [
        ":common",
        "//upb/reflection:reflection",
        "//upb/reflection:descriptor_upb_proto",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//upb:base",
        "//upb:mini_descriptor",
        "//upb:mini_table",
        "//upb:port",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "keywords",
    srcs = [
        "keywords.cc",
    ],
    hdrs = [
        "keywords.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//src/google/protobuf/compiler/hpb:__pkg__"],
)

bootstrap_cc_library(
    name = "plugin",
    hdrs = [
        "plugin.h",
    ],
    bootstrap_deps = [
        ":plugin_upb_proto",
        "//upb/reflection:descriptor_upb_proto",
        "//upb/reflection:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//upb:friend_generators"],
    deps = [
        "//upb:port",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/strings",
    ],
)

bootstrap_cc_library(
    name = "names",
    srcs = [
        "names.cc",
    ],
    hdrs = [
        "names.h",
    ],
    bootstrap_deps = [
        "//upb/reflection:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//upb:friend_generators"],
    deps = [
        "//:protobuf",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "mangle",
    srcs = ["mangle.cc"],
    hdrs = ["mangle.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = [
        "//rust:__subpackages__",
        "//upb:friends",
    ],
    deps = [
        "//src/google/protobuf:port",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "libupb_generator.so",
    srcs = ["upb_generator_so.c"],
    compatible_with = [],
    copts = UPB_DEFAULT_COPTS + ["-DUPB_BUILD_API"],
    linkshared = 1,
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":upbdev",
        "//upb:port",
    ],
)

cc_library(
    name = "upbdev",
    srcs = [
        "code_generator_request.c",
        "code_generator_request.h",
        "get_used_fields.c",
        "upbdev.c",
    ],
    hdrs = [
        "get_used_fields.h",
        "upbdev.h",
    ],
    compatible_with = [],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:private"],
    deps = [
        ":code_generator_request_upb_proto",
        ":code_generator_request_upb_proto_reflection",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf/compiler:plugin_upb_c_proto",
        "//src/google/protobuf/compiler:plugin_upb_reflection_proto",
        "//upb:base",
        "//upb:json",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_descriptor",
        "//upb:mini_table",
        "//upb:port",
        "//upb:reflection",
        "//upb:wire",
    ],
)
