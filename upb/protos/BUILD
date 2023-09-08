# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)
load(
    "//protos/bazel:upb_cc_proto_library.bzl",
    "upb_cc_proto_library_copts",
)

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
# end:google_only

licenses(["notice"])

cc_library(
    name = "repeated_field",
    hdrs = [
        "repeated_field.h",
        "repeated_field_iterator.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":protos",
        ":protos_traits",
        "//:base",
        "//:collections",
        "//:collections_internal",
        "//:mem",
        "//:message_copy",
        "//:message_types",
        "//:port",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "protos",
    srcs = [
        "protos.cc",
    ],
    hdrs = [
        "protos.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":protos_extension_lock",
        "//:base",
        "//:mem",
        "//:message_accessors_internal",
        "//:message_copy",
        "//:message_internal",
        "//:message_promote",
        "//:message_types",
        "//:mini_table",
        "//:wire",
        "//:wire_types",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)

# Internally used type traits.
cc_library(
    name = "protos_traits",
    hdrs = [
        "protos_traits.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:private"],
)

cc_library(
    name = "protos_internal",
    hdrs = ["protos_internal.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":protos",
        "//:mem",
        "//:message",
        "//:mini_table",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings:str_format",
    ],
)

cc_library(
    name = "protos_extension_lock",
    srcs = ["protos_extension_lock.cc"],
    hdrs = ["protos_extension_lock.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//:message",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/synchronization",
    ],
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_protos_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = [
        "protos_internal.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":protos",
        ":protos_internal",
        ":repeated_field",
        "//:mem",
        "//:message",
    ],
)

cc_test(
    name = "protos_internal_test",
    srcs = ["protos_internal_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":protos_internal",
        "//:mem",
        "//protos_generator/tests:test_model_upb_cc_proto",
        "//protos_generator/tests:test_model_upb_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

upb_cc_proto_library_copts(
    name = "upb_cc_proto_library_copts__for_generated_code_only_do_not_use",
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
)

cc_test(
    name = "repeated_field_iterator_test",
    srcs = ["repeated_field_iterator_test.cc"],
    deps = [
        ":repeated_field",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "protos_extension_lock_test",
    srcs = ["protos_extension_lock_test.cc"],
    deps = [
        "//:mem",
        "//protos",
        "//protos:protos_extension_lock",
        "//protos_generator/tests:test_model_upb_cc_proto",
        "@com_google_absl//absl/hash",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_googletest//:gtest_main",
    ],
)
