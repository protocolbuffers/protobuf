# An encoder/decoder for AppsJSPB format.
# Note that this is currently experimental and should not be used.

load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")
load("//upb/bazel:upb_minitable_proto_library.bzl", "upb_minitable_proto_library")
load(
    "//upb/bazel:upb_proto_library.bzl",
    "upb_proto_library",
)

cc_library(
    name = "jspb",
    srcs = [
        "decode.c",
        "encode.c",
    ],
    hdrs = [
        "decode.h",
        "encode.h",
    ],
    compatible_with = ["//buildenv/target:non_prod"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__pkg__"],
    deps = [
        "//upb:base",
        "//upb:collections",
        "//upb:collections_internal",
        "//upb:lex",
        "//upb:mem",
        "//upb:message",
        "//upb:message_accessors_internal",
        "//upb:message_types",
        "//upb:mini_table",
        "//upb:mini_table_internal",
        "//upb:port",
    ],
)

cc_test(
    name = "decode_test",
    srcs = ["decode_test.cc"],
    deps = [
        ":jspb",
        ":test_upb_proto",
        ":test_upb_proto_minitable",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "encode_test",
    srcs = ["encode_test.cc"],
    deps = [
        ":jspb",
        ":test_upb_proto",
        ":test_upb_proto_minitable",
        "//upb:base",
        "//upb:mem",
        "//upb:mini_table",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "fuzz_test",
    srcs = ["fuzz_test.cc"],
    deps = [
        ":jspb",
        "//testing/fuzzing:fuzztest",
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
        "//upb/upb/test:fuzz_util",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["test.proto"],
    deps = ["//:struct_proto"],
)

upb_proto_library(
    name = "test_upb_proto",
    testonly = 1,
    deps = [":test_proto"],
)

upb_minitable_proto_library(
    name = "test_upb_proto_minitable",
    testonly = 1,
    deps = [":test_proto"],
)
