# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//conformance:conformance.bzl", "conformance_test", "failure_lists")
load("//upb/bazel:copts.bzl", "UPB_DEFAULT_COPTS", "UPB_DEFAULT_FEATURES")
load(
    "//upb/bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
    "upb_proto_reflection_library",
)

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

# The upb testee for `conformance_test(in_process = True)`: defines
# MakeTesteeRunner() (//conformance:testee_runner) on top
# of :conformance_upb_harness, so the suites call upb in their own process
# instead of forking a conformance_upb binary.  One library per def pool setup,
# from the same source: UPB_CONFORMANCE_REBUILD_MINITABLES picks the variant
# (0: the generated mini tables, as conformance_upb; 1: mini tables rebuilt at
# runtime, as conformance_upb_dynamic_minitable) and its name in messages.  A
# conformance_test() links exactly one of them.
cc_library(
    name = "upb_in_process_testee",
    testonly = 1,
    srcs = ["upb_in_process_testee.cc"],
    local_defines = ["UPB_CONFORMANCE_REBUILD_MINITABLES=0"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_harness",
        "//conformance:test_runner",
        "//conformance:testee_runner",
        "//upb/base",
        "//upb/mem",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
    ],
    alwayslink = 1,
)

cc_library(
    name = "upb_dynamic_minitable_in_process_testee",
    testonly = 1,
    srcs = ["upb_in_process_testee.cc"],
    local_defines = ["UPB_CONFORMANCE_REBUILD_MINITABLES=1"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_harness",
        "//conformance:test_runner",
        "//conformance:testee_runner",
        "//upb/base",
        "//upb/mem",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@abseil-cpp//absl/types:span",
    ],
    alwayslink = 1,
)

# One test per variant library, built with the same define so that the
# expected variant name follows the library under test.
cc_test(
    name = "upb_in_process_testee_test",
    srcs = ["upb_in_process_testee_test.cc"],
    local_defines = ["UPB_CONFORMANCE_REBUILD_MINITABLES=0"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_proto",
        ":upb_in_process_testee",
        "//conformance:binary_wireformat",
        "//conformance:test_runner",
        "//conformance:testee_runner",
        "//upb/base",
        "//upb/mem",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "upb_dynamic_minitable_in_process_testee_test",
    srcs = ["upb_in_process_testee_test.cc"],
    local_defines = ["UPB_CONFORMANCE_REBUILD_MINITABLES=1"],
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    deps = [
        ":conformance_upb_proto",
        ":upb_dynamic_minitable_in_process_testee",
        "//conformance:binary_wireformat",
        "//conformance:test_runner",
        "//conformance:testee_runner",
        "//upb/base",
        "//upb/mem",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

# The upb testee as a binary speaking the conformance protocol on its
# stdin/stdout, for the forked conformance_test()s below.
cc_binary(
    name = "conformance_upb",
    testonly = 1,
    srcs = ["conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS,
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

# The expected-failure lists of the conformance_test()s below, one file per
# test and suite (e.g. failure_lists/upb_json.txt).  Update them with
# `blaze run :<test>_<suite>_test -- --fix`; see
# //conformance:conformance.bzl.
failure_lists(name = "failure_lists")

# The protobuf conformance tests (upb_binary_test, upb_json_test,
# upb_performance_test and upb_text_test) against the upb testee, linked into
# each test (in_process) rather than forked as conformance_upb.
conformance_test(
    name = "upb",
    failure_lists = ":failure_lists",
    in_process = True,
    maximum_edition = "2023",
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    testee = ":upb_in_process_testee",
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

# The same tests against the testee that rebuilds its mini tables at runtime.
# Deliberately still forked: it exercises the same harness as :upb through the
# other transport, and keeps the conformance_upb binaries' main() and framing
# built and run by a non-manual test.  To run it in-process too, set
# `in_process = True, testee = ":upb_dynamic_minitable_in_process_testee"`.
conformance_test(
    name = "upb_dynamic_minitable",
    failure_lists = ":failure_lists",
    maximum_edition = "2023",
    target_compatible_with = select({
        "@platforms//os:windows": ["@platforms//:incompatible"],
        "//conditions:default": [],
    }),
    testee = ":conformance_upb_dynamic_minitable",
)
