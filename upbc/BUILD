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
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)
load(
    "//upbc:bootstrap_compiler.bzl",
    "bootstrap_cc_binary",
    "bootstrap_cc_library",
    "bootstrap_upb_proto_library",
)

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

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

bootstrap_upb_proto_library(
    name = "plugin_upb_proto",
    base_dir = "",
    # TODO(b/289127200): Export 'net/proto2/proto/descriptor.upb.h' and remove "-layering_check".
    features = ["-layering_check"],
    google3_src_files = [
        "net/proto2/compiler/proto/profile.proto",
        "third_party/protobuf/compiler/plugin.proto",
    ],
    google3_src_rules = [
        "//net/proto2/proto:descriptor_proto_source",
        "//net/proto2/compiler/proto:profile.proto",
        "//third_party/protobuf/compiler:plugin_proto_source",
    ],
    oss_src_files = ["google/protobuf/compiler/plugin.proto"],
    oss_src_rules = [
        "@com_google_protobuf//:descriptor_proto_srcs",
        "@com_google_protobuf//src/google/protobuf/compiler:plugin_proto_src",
    ],
    oss_strip_prefix = "third_party/protobuf/github/bootstrap/src",
    proto_lib_deps = ["@com_google_protobuf//:compiler_plugin_proto"],
    visibility = ["//:friends"],
    deps = ["//:descriptor_upb_proto"],
)

upb_proto_reflection_library(
    name = "plugin_upb_proto_reflection",
    visibility = ["//:friends"],
    deps = ["@com_google_protobuf//:compiler_plugin_proto"],
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
        "//:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//protos_generator:__pkg__"],
    deps = [
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
        "//:reflection",
        "//:descriptor_upb_proto",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//protos_generator:__pkg__"],
    deps = [
        "//:mini_table",
        "//:mini_table_internal",
        "//:port",
        "//:upb",
        "//upb/mini_descriptor",
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
    visibility = ["//protos_generator:__pkg__"],
)

bootstrap_cc_library(
    name = "plugin",
    hdrs = [
        "plugin.h",
    ],
    bootstrap_deps = [
        ":plugin_upb_proto",
        "//:descriptor_upb_proto",
        "//:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//protos_generator:__pkg__"],
    deps = [
        "//:port",
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
        "//:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//protos_generator:__pkg__"],
    deps = [
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//src/google/protobuf/compiler:code_generator",
    ],
)

cc_binary(
    name = "libupbc.so",
    srcs = ["upbc_so.c"],
    copts = UPB_DEFAULT_COPTS + ["-DUPB_BUILD_API"],
    linkshared = 1,
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":upbdev",
        "//:port",
    ],
)

cc_library(
    name = "upbdev",
    srcs = [
        "code_generator_request.c",
        "code_generator_request.h",
        "upbdev.c",
    ],
    hdrs = [
        "upbdev.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:private"],
    deps = [
        ":code_generator_request_upb_proto",
        ":code_generator_request_upb_proto_reflection",
        ":plugin_upb_proto",
        ":plugin_upb_proto_reflection",
        "//:base",
        "//:json",
        "//:mem",
        "//:mini_table",
        "//:port",
        "//:reflection",
        "//upb/json",
        "//upb/mini_descriptor",
    ],
)

bootstrap_cc_binary(
    name = "protoc-gen-upb",
    srcs = ["protoc-gen-upb.cc"],
    bootstrap_deps = [
        ":common",
        ":file_layout",
        ":names",
        ":plugin",
        ":plugin_upb_proto",
        "//:descriptor_upb_proto",
        "//:reflection",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//:base",
        "//:mem",
        "//:mini_table_internal",
        "//:port",
        "//:wire_types",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "protoc-gen-upbdefs",
    srcs = [
        "protoc-gen-upbdefs.cc",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    # To work around the following link error from ABSL:
    # /usr/bin/x86_64-linux-gnu-ld: bazel-out/k8-opt-exec-2B5CBBC6-ST-c1776f9924ec/bin/external/com_google_absl/absl/time/libtime.a(duration.o): undefined reference to symbol 'floor@@GLIBC_2.2.5'
    # /usr/bin/x86_64-linux-gnu-ld: /opt/manylinux/2014/x86_64/lib64/libm.so.6: error adding symbols: DSO missing from command line
    # clang-14: error: linker command failed with exit code 1 (use -v to see invocation)
    linkopts = ["-lm"],
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        ":file_layout",
        ":plugin",
        "//:descriptor_upb_proto",
        "//:reflection",
        "//upb/util:def_to_proto",
    ],
)

cc_binary(
    name = "protoc-gen-upbdev",
    srcs = [
        "protoc-gen-upbdev.cc",
        "subprocess.cc",
        "subprocess.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    visibility = ["//visibility:public"],
    deps = [
        ":plugin_upb_proto",
        ":upbdev",
        "//:port",
        "@com_google_absl//absl/log:absl_log",
        "@com_google_absl//absl/strings",
    ],
)
