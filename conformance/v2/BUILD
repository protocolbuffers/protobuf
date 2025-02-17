load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("//conformance/v2:conformance.bzl", "conformance_suite", "conformance_test", "failure_lists")

cc_library(
    name = "binary_wireformat",
    testonly = True,
    srcs = ["binary_wireformat.cc"],
    hdrs = ["binary_wireformat.h"],
    deps = [
        "//src/google/protobuf:endian",
        "//src/google/protobuf:protobuf_lite",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "params",
    testonly = True,
    hdrs = ["params.h"],
    deps = [
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//editions:test_messages_proto2_editions_cc_proto",
        "//editions:test_messages_proto3_editions_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_legacy",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "naming",
    testonly = True,
    hdrs = ["naming.h"],
    deps = [
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_legacy",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "testee",
    testonly = True,
    srcs = ["testee.cc"],
    hdrs = ["testee.h"],
    deps = [
        ":binary_wireformat",
        ":naming",
        "//conformance:conformance_cc_proto",
        "//conformance:conformance_test_lib",
        "//src/google/protobuf",
        "//src/google/protobuf:endian",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "matchers",
    testonly = True,
    srcs = ["matchers.cc"],
    hdrs = ["matchers.h"],
    deps = [
        ":global_test_environment",
        ":test_manager",
        ":testee",
        "//conformance:conformance_cc_proto",
        "//src/google/protobuf",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/memory",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "test_manager",
    testonly = True,
    srcs = ["test_manager.cc"],
    hdrs = ["test_manager.h"],
    deps = [
        "//conformance:conformance_cc_proto",
        "//conformance:conformance_test_lib",
        "@abseil-cpp//absl/container:btree",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:flat_hash_set",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_library(
    name = "global_test_environment",
    testonly = True,
    hdrs = ["global_test_environment.h"],
    deps = [
        ":test_manager",
        ":testee",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "global_test_environment_main",
    testonly = True,
    srcs = ["global_test_environment_main.cc"],
    deps = [
        ":global_test_environment",
        ":test_manager",
        ":testee",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log:absl_check",
        "@abseil-cpp//absl/log:absl_log",
        "@abseil-cpp//absl/status:status_matchers",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

conformance_suite(
    name = "binary",
    srcs = ["binary_test.cc"],
    deps = [
        ":binary_wireformat",
        ":global_test_environment",
        ":matchers",
        ":naming",
        ":params",
        ":testee",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:test_textproto",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

conformance_suite(
    name = "text",
    srcs = ["text_test.cc"],
    deps = [
        ":binary_wireformat",
        ":global_test_environment",
        ":matchers",
        ":params",
        ":testee",
        "//:test_messages_proto2_cc_proto",
        "//:test_messages_proto3_cc_proto",
        "//conformance/test_protos:test_messages_edition2023_cc_proto",
        "//src/google/protobuf:test_textproto",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
    ],
)

conformance_test(
    name = "cpp",
    failure_lists = ":failure_lists",
    testee = "//conformance:conformance_cpp",
)

conformance_test(
    name = "java",
    failure_lists = ":failure_lists",
    testee = "//conformance:conformance_java",
)

conformance_test(
    name = "java_lite",
    failure_lists = ":failure_lists",
    testee = "//conformance:conformance_java_lite",
)

failure_lists(name = "failure_lists")

bzl_library(
    name = "conformance_bzl",
    srcs = ["conformance.bzl"],
    visibility = ["//visibility:private"],
)
