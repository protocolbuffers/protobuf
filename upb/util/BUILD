
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

cc_library(
    name = "required_fields",
    srcs = ["required_fields.c"],
    hdrs = ["required_fields.h"],
    deps = ["//:reflection"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "required_fields_test_proto",
    srcs = ["required_fields_test.proto"],
)

upb_proto_library(
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
        "@com_google_googletest//:gtest_main",
        ":required_fields",
        ":required_fields_test_upb_proto",
        ":required_fields_test_upb_proto_reflection",
    ],
)
