# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//rules:common_settings.bzl", "string_flag")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

string_flag(
    name = "hpb_backend",
    build_setting_default = "upb",
    values = [
        "upb",
        "cpp",
    ],
    visibility = ["//visibility:public"],
)

config_setting(
    name = "hpb_backend_upb",
    flag_values = {
        ":hpb_backend": "upb",
    },
    visibility = ["//visibility:public"],
)

config_setting(
    name = "hpb_backend_cpp",
    flag_values = {
        ":hpb_backend": "cpp",
    },
    visibility = ["//visibility:public"],
)

cc_library(
    name = "hpb",
    hdrs = [
        "hpb.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":arena",
        ":extension",
        ":multibackend",
        ":options",
        ":ptr",
        ":status",
        "//hpb/internal",
        "//hpb/internal:message_lock",
        "//hpb/internal:template_help",
        "@abseil-cpp//absl/base:core_headers",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ] + select({
        ":hpb_backend_cpp": [
            "//hpb/backend/cpp",
        ],
        ":hpb_backend_upb": [
            "//hpb/backend/upb",
            "//hpb/backend/upb:interop",
            "//upb/mini_table",
            "//upb/wire",
        ],
    }),
)

cc_library(
    name = "multibackend",
    hdrs = [
        "multibackend.h",
    ],
    defines = select({
        ":hpb_backend_upb": ["HPB_INTERNAL_BACKEND=HPB_INTERNAL_BACKEND_UPB"],
        ":hpb_backend_cpp": ["HPB_INTERNAL_BACKEND=HPB_INTERNAL_BACKEND_CPP"],
    }),
    visibility = ["//hpb:__subpackages__"],
)

cc_library(
    name = "repeated_field",
    hdrs = [
        "repeated_field.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":multibackend",
        ":ptr",
        "@abseil-cpp//absl/strings",
    ] + select({
        ":hpb_backend_cpp": [
            "//hpb/backend/cpp:repeated_field",
        ],
        ":hpb_backend_upb": [
            "//hpb/backend/upb:repeated_field",
        ],
    }),
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_hpb_support",
    visibility = [
        "//hpb/bazel:__pkg__",
        "//hpb_generator:__pkg__",
    ],
    deps = [
        ":hpb",
        ":repeated_field",
        "//hpb/internal:os_macros",
        "//upb/mem",
        "//upb/message",
    ],
)

cc_library(
    name = "requires",
    hdrs = ["requires.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "ptr",
    hdrs = ["ptr.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "arena",
    hdrs = ["arena.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":multibackend",
        "//hpb/backend:types",
    ] + select({
        ":hpb_backend_cpp": [
            "//src/google/protobuf:arena",
        ],
        ":hpb_backend_upb": [
            "//upb/mem",
        ],
    }),
)

cc_library(
    name = "status",
    srcs = ["status.cc"],
    hdrs = ["status.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":multibackend",
        "//hpb/backend:types",
        "//upb/wire",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_test(
    name = "status_test",
    srcs = ["status_test.cc"],
    deps = [
        ":ptr",
        ":status",
        "//hpb_generator/tests:test_model_hpb_proto",
        "//upb/wire",
        "@abseil-cpp//absl/status:statusor",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "extension",
    srcs = ["extension.cc"],
    hdrs = ["extension.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":arena",
        ":multibackend",
        ":ptr",
        "//hpb/internal:message_lock",
        "//hpb/internal:template_help",
        "//upb/message",
        "//upb/mini_table",
        "@abseil-cpp//absl/base:core_headers",
    ] + select({
        "//hpb:hpb_backend_cpp": [
        ],
        "//hpb:hpb_backend_upb": [
            "//hpb/backend/upb:extension",
            "//hpb/backend/upb:interop",
        ],
    }),
)

cc_library(
    name = "options",
    hdrs = ["options.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//hpb:extension",
        "//hpb:multibackend",
    ],
)
