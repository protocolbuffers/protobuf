# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

cc_binary(
    name = "protoc-gen-hpb",
    srcs = [
        "protoc-gen-hpb.cc",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    malloc = "@bazel_tools//tools/cpp:malloc",
    visibility = ["//visibility:public"],
    deps = [
        ":generator",
        "//src/google/protobuf/compiler:plugin",
        "@abseil-cpp//absl/flags:flag",
    ],
)

proto_lang_toolchain(
    name = "toolchain",
    command_line = select({
        "//hpb:hpb_backend_upb": "--hpb_out=$(OUT)",
        "//hpb:hpb_backend_cpp": "--hpb_out=backend=cpp:$(OUT)",
    }),
    output_files = "multiple",
    plugin = ":protoc-gen-hpb",
    plugin_format_flag = "--plugin=protoc-gen-hpb=%s",
    progress_message = "Generating hpb protocol buffers",
    runtime = "//hpb:generated_hpb_support",
    visibility = ["//hpb/bazel:__pkg__"],
)

cc_library(
    name = "generator",
    srcs = [
        "gen_accessors.cc",
        "gen_accessors.h",
        "gen_enums.cc",
        "gen_enums.h",
        "gen_extensions.cc",
        "gen_extensions.h",
        "gen_messages.cc",
        "gen_messages.h",
        "gen_repeated_fields.cc",
        "gen_repeated_fields.h",
        "generator.cc",
    ],
    hdrs = [
        "generator.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:private"],
    deps = [
        ":context",
        ":gen_utils",
        ":keywords",
        ":names",
        "//src/google/protobuf",
        "//src/google/protobuf/compiler:code_generator",
        "//src/google/protobuf/compiler:code_generator_lite",
        "//src/google/protobuf/compiler:plugin",
        "//upb_generator:common",
        "//upb_generator:file_layout",
        "//upb_generator/c:names",
        "//upb_generator/minitable:names",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "keywords",
    srcs = ["keywords.cc"],
    hdrs = ["keywords.h"],
    visibility = ["//visibility:private"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "gen_utils",
    srcs = ["gen_utils.cc"],
    hdrs = ["gen_utils.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//src/google/protobuf",
        "//src/google/protobuf/compiler:code_generator",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "names",
    srcs = ["names.cc"],
    hdrs = ["names.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":context",
        ":keywords",
        "//src/google/protobuf",
        "//src/google/protobuf/compiler:code_generator",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "context",
    hdrs = ["context.h"],
    deps = [
        "//src/google/protobuf",
        "//src/google/protobuf/io",
        "//src/google/protobuf/io:printer",
        "//upb/reflection",
        "//upb_generator/common:cpp_to_upb_def",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:source_location",
        "@abseil-cpp//absl/types:span",
    ],
)
