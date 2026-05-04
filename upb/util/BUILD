load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//upb/bazel:fuzz_test.bzl", "fuzz_test")
load(
    "//upb/bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
    "upb_proto_reflection_library",
)

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

# Def to Proto

cc_library(
    name = "def_to_proto",
    srcs = ["def_to_proto.c"],
    hdrs = ["def_to_proto.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/port",
        "//upb/reflection",
        "//upb/reflection:internal",
    ],
)

proto_library(
    name = "def_to_proto_test_proto",
    srcs = [
        "def_to_proto_public_import_test.proto",
        "def_to_proto_regular_import_test.proto",
        "def_to_proto_test.proto",
    ],
)

upb_c_proto_library(
    name = "def_to_proto_test_upb_proto",
    deps = ["def_to_proto_test_proto"],
)

upb_proto_reflection_library(
    name = "def_to_proto_test_upb_proto_reflection",
    deps = ["def_to_proto_test_proto"],
)

cc_test(
    name = "def_to_proto_test",
    srcs = ["def_to_proto_test.cc"],
    deps = [
        ":def_to_proto",
        ":def_to_proto_editions_test_upb_proto",
        ":def_to_proto_editions_test_upb_proto_reflection",
        ":def_to_proto_test_upb_proto",
        ":def_to_proto_test_upb_proto_reflection",
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf:descriptor_upb_reflection_proto",
        "//src/google/protobuf/util:differencer",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/reflection",
        "//upb/reflection:internal",
        "//upb/test:parse_text_proto",
        "//upb/wire",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "def_to_proto_fuzz_impl",
    testonly = 1,
    srcs = ["def_to_proto_fuzz_impl.cc"],
    hdrs = ["def_to_proto_fuzz_impl.h"],
    visibility = ["//upb:__pkg__"],
    deps = [
        ":def_to_proto",
        "//src/google/protobuf",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf/util:differencer",
        "//upb/base",
        "//upb/mem",
        "//upb/reflection:internal",
        "//upb/test:parse_text_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "def_to_proto_fuzz_regression_test",
    srcs = ["def_to_proto_fuzz_regression_test.cc"],
    deps = [
        ":def_to_proto_fuzz_impl",
        "//upb/test:parse_text_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

fuzz_test(
    name = "def_to_proto_fuzz",
    srcs = ["def_to_proto_fuzz_test.cc"],
    deps = [
        ":def_to_proto_fuzz_impl",
        "//src/google/protobuf",
    ],
)

# Required fields

cc_library(
    name = "required_fields",
    srcs = ["required_fields.c"],
    hdrs = ["required_fields.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/port",
        "//upb/reflection",
    ],
)

proto_library(
    name = "def_to_proto_editions_test_proto",
    srcs = ["def_to_proto_editions_test.proto"],
)

upb_c_proto_library(
    name = "def_to_proto_editions_test_upb_proto",
    deps = ["def_to_proto_editions_test_proto"],
)

upb_proto_reflection_library(
    name = "def_to_proto_editions_test_upb_proto_reflection",
    deps = ["def_to_proto_editions_test_proto"],
)

proto_library(
    name = "required_fields_test_proto",
    srcs = [
        "required_fields_editions_test.proto",
        "required_fields_test.proto",
    ],
)

upb_c_proto_library(
    name = "required_fields_test_upb_proto",
    deps = ["required_fields_test_proto"],
)

upb_proto_reflection_library(
    name = "required_fields_test_upb_proto_reflection",
    deps = ["required_fields_test_proto"],
)

cc_test(
    name = "required_fields_test",
    srcs = ["required_fields_test.cc"],
    deps = [
        ":required_fields",
        ":required_fields_test_upb_proto",
        ":required_fields_test_upb_proto_reflection",
        "//upb/base",
        "//upb/json",
        "//upb/mem",
        "//upb/reflection",
        "//upb/reflection:internal",
        "@abseil-cpp//absl/strings",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

filegroup(
    name = "source_files",
    srcs = [
        "def_to_proto.c",
        "def_to_proto.h",
        "required_fields.c",
        "required_fields.h",
    ],
    visibility = ["//python/dist:__pkg__"],
)

filegroup(
    name = "test_srcs",
    srcs = glob(
        [
            "**/*test.cc",
        ],
        exclude = ["def_to_proto_fuzz_test.cc"],
    ),
    visibility = ["//upb:__pkg__"],
)

filegroup(
    name = "test_protos",
    srcs = glob(
        [
            "**/*test.proto",
        ],
    ),
    visibility = ["//upb:__pkg__"],
)
