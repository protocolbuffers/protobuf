
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

# Def to Proto

cc_library(
    name = "def_to_proto",
    srcs = ["def_to_proto.c"],
    hdrs = ["def_to_proto.h"],
    deps = ["//:reflection"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "def_to_proto_test_proto",
    srcs = [
        "def_to_proto_test.proto",
        "def_to_proto_regular_import_test.proto",
        "def_to_proto_public_import_test.proto",
        "def_to_proto_weak_import_test.proto",
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
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "//:descriptor_upb_proto_reflection",
        "@com_google_protobuf//:protobuf",
        ":def_to_proto",
        ":def_to_proto_test_upb_proto",
        ":def_to_proto_test_upb_proto_reflection",
    ],
)

# Required fields

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
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "//:json",
        ":required_fields",
        ":required_fields_test_upb_proto",
        ":required_fields_test_upb_proto_reflection",
    ],
)

# Compare

cc_library(
    name = "compare",
    srcs = ["compare.c"],
    hdrs = ["compare.h"],
    deps = ["//:reflection"],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "compare_test",
    srcs = ["compare_test.cc"],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        ":compare",
    ],
)
