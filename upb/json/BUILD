# TODO: describe this package.

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//upb/bazel:copts.bzl", "UPB_DEFAULT_COPTS", "UPB_DEFAULT_FEATURES")
load("//upb/bazel:fuzz_test.bzl", "fuzz_test")
load(
    "//upb/bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
    "upb_proto_reflection_library",
)

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "json",
    srcs = [
        "decode.c",
        "encode.c",
    ],
    hdrs = [
        "decode.h",
        "encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    features = UPB_DEFAULT_FEATURES,
    visibility = ["//visibility:public"],
    deps = [
        "//upb/base",
        "//upb/lex",
        "//upb/mem",
        "//upb/message",
        "//upb/mini_table",
        "//upb/port",
        "//upb/reflection",
        "//upb/wire",
    ],
)

cc_test(
    name = "decode_test",
    srcs = ["decode_test.cc"],
    deps = [
        ":json",
        ":test_upb_proto",
        ":test_upb_proto_reflection",
        "//:struct_upb_proto",
        "//upb/base",
        "//upb/mem",
        "//upb/reflection",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "encode_test",
    srcs = ["encode_test.cc"],
    deps = [
        ":json",
        ":test_upb_proto",
        ":test_upb_proto_reflection",
        "//:struct_upb_proto",
        "//upb/base",
        "//upb/mem",
        "//upb/reflection",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["test.proto"],
    deps = ["//:struct_proto"],
)

upb_c_proto_library(
    name = "test_upb_proto",
    testonly = 1,
    deps = [":test_proto"],
)

upb_proto_reflection_library(
    name = "test_upb_proto_reflection",
    testonly = 1,
    deps = [":test_proto"],
)

cc_library(
    name = "fuzz_impl",
    testonly = 1,
    srcs = ["fuzz_impl.cc"],
    hdrs = ["fuzz_impl.h"],
    deps = [
        ":json",
        ":test_upb_proto",
        ":test_upb_proto_reflection",
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/mini_table",
        "//upb/reflection",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "fuzz_regression_test",
    srcs = ["fuzz_regression_test.cc"],
    deps = [
        ":fuzz_impl",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

fuzz_test(
    name = "fuzz",
    srcs = ["fuzz_test.cc"],
    deps = [
        ":fuzz_impl",
    ],
)

filegroup(
    name = "test_srcs",
    srcs = glob(
        [
            "**/*test.cc",
        ],
        exclude = ["fuzz_test.cc"],
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
