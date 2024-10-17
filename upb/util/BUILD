load(
    "//bazel:upb_proto_library.bzl",
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
        "//upb:base",
        "//upb:port",
        "//upb:reflection",
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

cc_library(
    name = "def_to_proto_test_lib",
    testonly = 1,
    hdrs = ["def_to_proto_test.h"],
    deps = [
        ":def_to_proto",
        "//:protobuf",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf/util:differencer",
        "//upb:base",
        "//upb:mem",
        "//upb/reflection:internal",
        "@com_google_googletest//:gtest",
    ],
)

cc_test(
    name = "def_to_proto_test",
    srcs = ["def_to_proto_test.cc"],
    deps = [
        ":def_to_proto",
        ":def_to_proto_editions_test_upb_proto",
        ":def_to_proto_editions_test_upb_proto_reflection",
        ":def_to_proto_test_lib",
        ":def_to_proto_test_upb_proto",
        ":def_to_proto_test_upb_proto_reflection",
        "//:protobuf",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf:descriptor_upb_reflection_proto",
        "//src/google/protobuf/util:differencer",
        "//upb:base",
        "//upb:mem",
        "//upb:reflection",
        "//upb/test:parse_text_proto",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

# Required fields

cc_library(
    name = "required_fields",
    srcs = ["required_fields.c"],
    hdrs = ["required_fields.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:port",
        "//upb:reflection",
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
        "//upb:base",
        "//upb:json",
        "//upb:mem",
        "//upb:reflection",
        "//upb/reflection:internal",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
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
    name = "test_util",
    srcs = glob(
        [
            "**/*test.h",
        ],
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
