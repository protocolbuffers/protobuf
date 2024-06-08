# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
    "upb_proto_reflection_library",
)
load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
    "make_shell_script",
)

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

licenses(["notice"])

config_setting(
    name = "windows",
    constraint_values = ["@platforms//os:windows"],
    visibility = ["//visibility:public"],
)

upb_c_proto_library(
    name = "conformance_upb_proto",
    testonly = 1,
    deps = ["//conformance:conformance_proto"],
)

upb_proto_reflection_library(
    name = "conformance_upb_proto_reflection",
    testonly = 1,
    deps = ["//conformance:conformance_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto2_upbdefs",
    testonly = 1,
    visibility = ["//visibility:private"],
    deps = ["//src/google/protobuf:test_messages_proto2_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto3_upbdefs",
    testonly = 1,
    visibility = ["//visibility:private"],
    deps = ["//src/google/protobuf:test_messages_proto3_proto"],
)

cc_binary(
    name = "conformance_upb",
    testonly = 1,
    srcs = ["conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS,
    data = ["conformance_upb_failures.txt"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_proto",
        ":conformance_upb_proto_reflection",
        ":test_messages_proto2_upbdefs",
        ":test_messages_proto3_upbdefs",
        "//editions:test_messages_proto2_editions_upbdefs",
        "//editions:test_messages_proto3_editions_upbdefs",
        "//upb:base",
        "//upb:json",
        "//upb:port",
        "//upb:reflection",
        "//upb:text",
        "//upb:wire",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb",
    out = "test_conformance_upb.sh",
    contents = "$1 --maximum_edition 2023 --enforce_recommended --failure_list $2 $3",
)

sh_test(
    name = "test_conformance_upb",
    srcs = ["test_conformance_upb.sh"],
    args = [
        "$(location //conformance:conformance_test_runner)",
        "$(location :conformance_upb_failures.txt)",
        "$(location :conformance_upb)",
    ],
    data = [
        "conformance_upb_failures.txt",
        ":conformance_upb",
        "//conformance:conformance_test_runner",
    ],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = ["@bazel_tools//tools/bash/runfiles"],
)

cc_binary(
    name = "conformance_upb_dynamic_minitable",
    testonly = 1,
    srcs = ["conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS + [
        "-DREBUILD_MINITABLES",
    ],
    data = ["conformance_upb_failures.txt"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_proto",
        ":conformance_upb_proto_reflection",
        ":test_messages_proto2_upbdefs",
        ":test_messages_proto3_upbdefs",
        "//editions:test_messages_proto2_editions_upbdefs",
        "//editions:test_messages_proto3_editions_upbdefs",
        "//upb:base",
        "//upb:json",
        "//upb:port",
        "//upb:reflection",
        "//upb:text",
        "//upb:wire",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb_dynamic_minitable",
    out = "test_conformance_upb_dynamic_minitable.sh",
    contents = "$1 --maximum_edition 2023 --enforce_recommended --failure_list $2 $3",
)

sh_test(
    name = "test_conformance_upb_dynamic_minitable",
    srcs = ["test_conformance_upb_dynamic_minitable.sh"],
    args = [
        "$(location //conformance:conformance_test_runner)",
        "$(location :conformance_upb_failures.txt)",
        "$(location :conformance_upb_dynamic_minitable)",
    ],
    data = [
        "conformance_upb_failures.txt",
        ":conformance_upb_dynamic_minitable",
        "//conformance:conformance_test_runner",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
