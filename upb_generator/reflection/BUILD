# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_CPPOPTS")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

cc_library(
    name = "names",
    srcs = ["names.cc"],
    hdrs = ["names.h"],
    deps = [
        "//upb/port",
        "//upb_generator/common:names",
        "@abseil-cpp//absl/strings",
    ],
)

cc_binary(
    name = "protoc-gen-upbdefs",
    srcs = [
        "context.h",
        "generator.cc",
        "header.cc",
        "header.h",
        "source.cc",
        "source.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = [
        "//pkg:__pkg__",
    ],
    deps = [
        ":names",
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf/compiler:code_generator",
        "//src/google/protobuf/compiler:plugin",
        "//src/google/protobuf/io",
        "//src/google/protobuf/io:printer",
        "//upb/mem",
        "//upb/port",
        "//upb/reflection",
        "//upb/util:def_to_proto",
        "//upb_generator:common",
        "//upb_generator:file_layout",
        "//upb_generator:plugin",
        "//upb_generator/common:names",
        "//upb_generator/minitable:names",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:cord",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
    ],
)

proto_lang_toolchain(
    name = "toolchain",
    command_line = "--upbdefs_out=$(OUT)",
    output_files = "multiple",
    plugin = ":protoc-gen-upbdefs",
    plugin_format_flag = "--plugin=protoc-gen-upbdefs=%s",
    progress_message = "Generating upb protos",
    runtime = "//upb/reflection:generated_reflection_support",
    visibility = ["//bazel:__pkg__"],
)
