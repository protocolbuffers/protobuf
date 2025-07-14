# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)
load("//upb_generator:bootstrap_compiler.bzl", "bootstrap_cc_binary")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

cc_library(
    name = "names",
    srcs = ["names.cc"],
    hdrs = ["names.h"],
    visibility = ["//src/google/protobuf/compiler/hpb:__pkg__"],
    deps = [
        ":names_internal",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "names_internal",
    srcs = ["names_internal.cc"],
    hdrs = ["names_internal.h"],
    deps = [
        "//upb_generator/common:names",
        "@abseil-cpp//absl/strings",
    ],
)

bootstrap_cc_binary(
    name = "protoc-gen-upb",
    srcs = ["generator.cc"],
    bootstrap_deps = [
        "//upb_generator:common",
        "//upb_generator:file_layout",
        "//upb_generator:plugin",
        "//upb/reflection:descriptor_upb_proto",
        "//upb/reflection:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = [
        "//pkg:__pkg__",
    ],
    deps = [
        ":names",
        ":names_internal",
        "//src/google/protobuf/compiler:code_generator",
        "//src/google/protobuf/compiler:plugin",
        "//upb/base",
        "//upb/mem",
        "//upb/mini_table",
        "//upb/port",
        "//upb/wire:reader",
        "//upb_generator/common:names",
        "//upb_generator/minitable:names",
        "//upb_generator/minitable:names_internal",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:cord",
    ],
)

proto_lang_toolchain(
    name = "toolchain",
    command_line = "--upb_out=$(OUT)",
    output_files = "multiple",
    plugin = ":protoc-gen-upb_stage1",
    plugin_format_flag = "--plugin=protoc-gen-upb=%s",
    progress_message = "Generating upb protos",
    runtime = "//upb:generated_code_support",
    visibility = ["//bazel:__pkg__"],
)
