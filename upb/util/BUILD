load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

licenses(["notice"])

# Def to Proto

cc_library(
    name = "def_to_proto",
    srcs = ["def_to_proto.c"],
    hdrs = ["def_to_proto.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//:port",
        "//:reflection",
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

upb_proto_library(
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
        ":def_to_proto_test_upb_proto",
        ":def_to_proto_test_upb_proto_reflection",
        "//:descriptor_upb_proto_reflection",
        "//:reflection",
        "//:upb",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

# Required fields

cc_library(
    name = "required_fields",
    srcs = ["required_fields.c"],
    hdrs = ["required_fields.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//:port",
        "//:reflection",
    ],
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
        ":required_fields",
        ":required_fields_test_upb_proto",
        ":required_fields_test_upb_proto_reflection",
        "//:json",
        "//:reflection",
        "//:upb",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

# Compare

cc_library(
    name = "compare",
    srcs = ["compare.c"],
    hdrs = ["compare.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//:port",
        "//:reflection",
    ],
)

cc_test(
    name = "compare_test",
    srcs = ["compare_test.cc"],
    deps = [
        ":compare",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)
