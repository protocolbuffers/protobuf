# Copyright (c) 2009-2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_CPPOPTS")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

cc_library(
    name = "names",
    srcs = ["names.cc"],
    hdrs = ["names.h"],
    deps = [
        "//upb:port",
        "//upb_generator/common:names",
        "@abseil-cpp//absl/strings",
    ],
)

cc_binary(
    name = "protoc-gen-upbdefs",
    linkopts = ["-lm"],
    visibility = ["//net/proto2/contrib/protoc_explorer:__pkg__"],
    deps = [":generator_with_main"],
)

# TODO: This wrapper lib is a hack that we need because of how CcInfo works in Bazel 6.
# In Bazel 7, our cmake dependency scraping works fine with cc_binary.
cc_library(
    name = "generator_with_main",
    srcs = ["generator.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//pkg:__pkg__"],
    # To work around the following link error from ABSL:
    # /usr/bin/x86_64-linux-gnu-ld: bazel-out/k8-opt-exec-2B5CBBC6-ST-c1776f9924ec/bin/external/com_google_absl/absl/time/libtime.a(duration.o): undefined reference to symbol 'floor@@GLIBC_2.2.5'
    # /usr/bin/x86_64-linux-gnu-ld: /opt/manylinux/2014/x86_64/lib64/libm.so.6: error adding symbols: DSO missing from command line
    # clang-14: error: linker command failed with exit code 1 (use -v to see invocation)
    deps = [
        ":names",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf/compiler:code_generator",
        "//src/google/protobuf/compiler:plugin",
        "//upb:mem",
        "//upb:port",
        "//upb:reflection",
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
        "@abseil-cpp//absl/strings:string_view",
    ],
)

proto_lang_toolchain(
    name = "toolchain",
    command_line = "--upbdefs_out=$(OUT)",
    output_files = "multiple",
    plugin = ":protoc-gen-upbdefs",
    plugin_format_flag = "--plugin=protoc-gen-upbdefs=%s",
    progress_message = "Generating upb protos",
    runtime = "//upb:generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    # TODO: Restrict to "//bazel:__pkg__" once we are on Bazel >=6.5.
    visibility = ["//visibility:public"],
)
