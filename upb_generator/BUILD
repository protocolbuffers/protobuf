# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_CPPOPTS")
load(
    "//upb_generator:bootstrap_compiler.bzl",
    "bootstrap_cc_library",
    "bootstrap_upb_proto_library",
)

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

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
        "//src/google/protobuf:port",
        "//upb:mini_table",
        "//upb:port",
        "@abseil-cpp//absl/strings",
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
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings",
    ],
)

bootstrap_cc_library(
    name = "plugin",
    srcs = [
        "plugin.cc",
    ],
    hdrs = [
        "plugin.h",
    ],
    bootstrap_deps = [
        ":plugin_upb_proto",
        "//upb/reflection:descriptor_upb_proto",
        "//upb/reflection:reflection",
        "//upb_generator:file_layout",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//upb:friend_generators"],
    deps = [
        "//src/google/protobuf",
        "//src/google/protobuf/compiler:code_generator_lite",
        "//upb:base",
        "//upb:mem",
        "//upb:port",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(name = "mangle")
