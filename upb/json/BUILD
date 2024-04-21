# TODO: describe this package.

load(
    "//bazel:upb_proto_library.bzl",
    "upb_c_proto_library",
    "upb_proto_reflection_library",
)
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

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
        "//upb:base",
        "//upb:mem",
        "//upb:message",
        "//upb:mini_table",
        "//upb:port",
        "//upb:reflection",
        "//upb:wire",
        "//upb/lex",
    ],
)

cc_test(
    name = "decode_test",
    srcs = ["decode_test.cc"],
    deps = [
        ":json",
        ":struct_upb_proto",
        ":test_upb_proto",
        ":test_upb_proto_reflection",
        "//upb:base",
        "//upb:mem",
        "//upb:reflection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "encode_test",
    srcs = ["encode_test.cc"],
    deps = [
        ":json",
        ":struct_upb_proto",
        ":test_upb_proto",
        ":test_upb_proto_reflection",
        "//upb:base",
        "//upb:mem",
        "//upb:reflection",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
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

# TODO: This target arguably belongs in //google/protobuf/BUILD
upb_c_proto_library(
    name = "struct_upb_proto",
    testonly = 1,
    deps = ["//:struct_proto"],
)

# begin:google_only
# cc_test(
#     name = "fuzz_test",
#     srcs = ["fuzz_test.cc"],
#     deps = [
#         ":json",
#         ":test_upb_proto",
#         ":test_upb_proto_reflection",
#         "@com_google_googletest//:gtest", "@com_google_googletest//:gtest_main",
#         "//testing/fuzzing:fuzztest",
#         "//upb:base",
#         "//upb:mem",
#         "//upb:message",
#         "//upb:mini_table",
#         "//upb:reflection",
#     ],
# )
# end:google_only

# begin:github_only
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
# end:github_only
