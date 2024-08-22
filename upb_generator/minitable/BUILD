# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_CPPOPTS")
load(
    "//upb_generator:bootstrap_compiler.bzl",
    "bootstrap_cc_binary",
    "bootstrap_cc_library",
)

bootstrap_cc_library(
    name = "generator",
    srcs = ["generator.cc"],
    hdrs = ["generator.h"],
    bootstrap_deps = [
        "//upb_generator:common",
        "//upb_generator:file_layout",
        "//upb_generator:names",
        "//upb_generator:plugin",
        "//upb_generator:plugin_upb_proto",
        "//upb/reflection:descriptor_upb_proto",
        "//upb/reflection:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        "//src/google/protobuf/compiler:code_generator",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "//upb:port",
        "//upb:wire_reader",
        "//upb/mini_table:internal",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/strings",
    ],
)

bootstrap_cc_binary(
    name = "protoc-gen-upb_minitable",
    bootstrap_deps = [
        ":generator_with_main",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = [
        "//editions/codegen_tests:__pkg__",
        "//net/proto2/contrib/protoc_explorer:__pkg__",
        "//third_party/prototiller/transformer:__pkg__",
    ],
)

# TODO: This wrapper lib is a hack that we need because of how CcInfo works in Bazel 6.
# In Bazel 7, our cmake dependency scraping works fine with cc_binary.
bootstrap_cc_library(
    name = "generator_with_main",
    srcs = ["main.cc"],
    bootstrap_deps = [
        "//upb_generator:file_layout",
        "//upb_generator:common",
        "//upb_generator:plugin",
        ":generator",
        "//upb/reflection:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//pkg:__pkg__"],
    deps = [
        "//upb:base",
        "//upb:port",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/strings",
    ],
)

proto_lang_toolchain(
    name = "toolchain",
    command_line = "--upb_minitable_out=$(OUT)",
    output_files = "multiple",
    plugin = ":protoc-gen-upb_minitable_stage1",
    plugin_format_flag = "--plugin=protoc-gen-upb_minitable=%s",
    progress_message = "Generating upb minitables",
    runtime = "//upb:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    # TODO: Restrict to "//bazel:__pkg__" once we are on Bazel >=6.5.
    visibility = ["//visibility:public"],
)
