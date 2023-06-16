# TODO(haberman): describe this package.

load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_reflection_library",
)

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
    visibility = ["//visibility:public"],
    deps = [
        "//:collections",
        "//:lex",
        "//:port",
        "//:reflection",
        "//:upb",
    ],
)

cc_test(
    name = "encode_test",
    srcs = ["encode_test.cc"],
    deps = [
        ":json",
        ":json_test_upb_proto",
        ":json_test_upb_proto_reflection",
        ":struct_upb_proto",
        "//:reflection",
        "//:upb",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "json_test_proto",
    testonly = 1,
    srcs = ["test.proto"],
    deps = ["@com_google_protobuf//:struct_proto"],
)

upb_proto_library(
    name = "json_test_upb_proto",
    testonly = 1,
    deps = [":json_test_proto"],
)

upb_proto_reflection_library(
    name = "json_test_upb_proto_reflection",
    testonly = 1,
    deps = [":json_test_proto"],
)

cc_test(
    name = "json_decode_test",
    srcs = ["decode_test.cc"],
    deps = [
        ":json",
        ":json_test_upb_proto",
        ":json_test_upb_proto_reflection",
        ":struct_upb_proto",
        "//:reflection",
        "//:upb",
        "@com_google_googletest//:gtest_main",
    ],
)

upb_proto_library(
    name = "struct_upb_proto",
    deps = ["@com_google_protobuf//:struct_proto"],
)
