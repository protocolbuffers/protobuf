# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_CPPOPTS")
load(
    "//upb_generator:bootstrap_compiler.bzl",
    "bootstrap_cc_binary",
    "bootstrap_cc_library",
)

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

bootstrap_cc_library(
    name = "generator",
    srcs = ["generator.cc"],
    hdrs = ["generator.h"],
    bootstrap_deps = [
        "//upb_generator:common",
        "//upb_generator:file_layout",
        "//upb/reflection:descriptor_upb_proto",
        "//upb/reflection:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":names",
        ":names_internal",
        "//src/google/protobuf/compiler:code_generator",
        "//src/google/protobuf/compiler:code_generator_lite",
        "//upb/base",
        "//upb/mem",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/wire:reader",
        "//upb/wire/decode_fast:select",
        "//upb_generator/common:names",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:cord",
    ],
)

cc_library(
    name = "names",
    srcs = ["names.cc"],
    hdrs = ["names.h"],
    visibility = [
        "//hpb_generator:__pkg__",
        "//src/google/protobuf/compiler/rust:__pkg__",
        "//third_party/kotlin/protobuf/generator/native:__pkg__",
        "//upb_generator:__subpackages__",
    ],
    deps = [
        ":names_internal",
        "//upb/port",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "names_internal",
    srcs = ["names_internal.cc"],
    hdrs = ["names_internal.h"],
    visibility = [
        # For bootstrapping.
        "//upb_generator/c:__pkg__",
    ],
    deps = [
        "//upb_generator/common:names",
        "@abseil-cpp//absl/strings",
    ],
)

bootstrap_cc_binary(
    name = "protoc-gen-upb_minitable",
    srcs = ["main.cc"],
    bootstrap_deps = [
        ":generator",
        "//upb_generator:file_layout",
        "//upb_generator:common",
        "//upb_generator:plugin",
        "//upb/reflection:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = [
        "//editions/codegen_tests:__pkg__",
        "//net/proto2/contrib/protoc_explorer:__pkg__",
        "//pkg:__pkg__",
        "//rust:__subpackages__",
        "//third_party/prototiller/transformer:__pkg__",
    ],
    deps = [
        ":names",
        ":names_internal",
        "//src/google/protobuf/compiler:code_generator",
        "//src/google/protobuf/compiler:code_generator_lite",
        "//src/google/protobuf/compiler:plugin",
        "//upb/base",
        "//upb/mem",
        "//upb/port",
        "//upb_generator/common:names",
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
    command_line = "--upb_minitable_out=$(OUT)",
    output_files = "multiple",
    plugin = ":protoc-gen-upb_minitable_stage1",
    plugin_format_flag = "--plugin=protoc-gen-upb_minitable=%s",
    progress_message = "Generating upb minitables",
    runtime = "//upb:generated_code_support",
    visibility = ["//bazel:__pkg__"],
)
