# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("//upb/bazel:copts.bzl", "UPB_DEFAULT_COPTS", "UPB_DEFAULT_FEATURES")
load(
    "//upb/bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
    "upb_proto_reflection_library",
)
load(":build_defs.bzl", "make_shell_script")

package(default_applicable_licenses = ["//:license"])

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

# The request handling of the upb testee, shared by the two conformance_upb
# binaries below (the stdin/stdout protocol around it) and usable in-process.
cc_library(
    name = "conformance_upb_harness",
    testonly = 1,
    srcs = ["conformance_upb_harness.c"],
    hdrs = ["conformance_upb_harness.h"],
    copts = UPB_DEFAULT_COPTS,
    features = UPB_DEFAULT_FEATURES,
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
        "//upb/base",
        "//upb/json",
        "//upb/mem",
        "//upb/message",
        "//upb/port",
        "//upb/reflection",
        "//upb/reflection:internal",
        "//upb/text",
        "//upb/wire",
    ],
)

cc_test(
    name = "conformance_upb_harness_test",
    srcs = ["conformance_upb_harness_test.cc"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_harness",
        ":conformance_upb_proto",
        "//conformance:binary_wireformat",
        "//upb/base",
        "//upb/mem",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "conformance_upb",
    testonly = 1,
    srcs = ["conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS,
    data = ["conformance_upb_failures.txt"],
    features = UPB_DEFAULT_FEATURES,
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_harness",
        "//upb/base",
        "//upb/mem",
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

# The same testee, with its mini tables rebuilt at runtime from the
# descriptors instead of the generated ones.
cc_binary(
    name = "conformance_upb_dynamic_minitable",
    testonly = 1,
    srcs = ["conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS + [
        "-DREBUILD_MINITABLES",
    ],
    data = ["conformance_upb_failures.txt"],
    features = UPB_DEFAULT_FEATURES,
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_harness",
        "//upb/base",
        "//upb/mem",
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

make_shell_script(
    name = "gen_test_conformance_upb_performance",
    out = "test_conformance_upb_performance.sh",
    contents = "$1 --performance --maximum_edition 2023 --enforce_recommended --failure_list $2 $3",
)

sh_test(
    name = "test_conformance_upb_performance",
    srcs = ["test_conformance_upb_performance.sh"],
    args = [
        "$(location //conformance:conformance_test_runner)",
        "$(location :conformance_upb_failures_performance.txt)",
        "$(location :conformance_upb)",
    ],
    data = [
        "conformance_upb_failures_performance.txt",
        ":conformance_upb",
        "//conformance:conformance_test_runner",
    ],
    # TODO Add upb conformance performance test back
    # The upb performance test has segvs with recursion limit test
    # cl/895387206 with --config=asan
    tags = [
        "manual",
        "notap",
    ],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = ["@bazel_tools//tools/bash/runfiles"],
)
