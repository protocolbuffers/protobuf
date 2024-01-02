# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
# end:google_only

cc_library(
    name = "string",
    hdrs = ["string.h"],
    deps = [
        "//upb:mem",
        "//upb:port",
    ],
)

cc_library(
    name = "tokenizer",
    srcs = ["tokenizer.c"],
    hdrs = ["tokenizer.h"],
    deps = [
        ":string",
        ":zero_copy_stream",
        "//upb:base",
        "//upb:mem",
        "//upb:port",
        "//upb/lex",
    ],
)

cc_library(
    name = "zero_copy_stream",
    hdrs = [
        "zero_copy_input_stream.h",
        "zero_copy_output_stream.h",
    ],
    deps = [
        "//upb:base",
        "//upb:mem",
        "//upb:port",
    ],
)

# ====================================================================

cc_library(
    name = "chunked_stream",
    testonly = 1,
    srcs = [
        "chunked_input_stream.c",
        "chunked_output_stream.c",
    ],
    hdrs = [
        "chunked_input_stream.h",
        "chunked_output_stream.h",
    ],
    deps = [
        ":zero_copy_stream",
        "//upb:mem",
        "//upb:port",
    ],
)

cc_test(
    name = "string_test",
    size = "small",
    srcs = ["string_test.cc"],
    deps = [
        ":string",
        "//upb:mem",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "tokenizer_test",
    size = "small",
    srcs = ["tokenizer_test.cc"],
    deps = [
        ":chunked_stream",
        ":string",
        ":tokenizer",
        ":zero_copy_stream",
        "//upb:mem",
        "//upb:port",
        "//upb/lex",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "zero_copy_stream_test",
    size = "small",
    srcs = [
        "zero_copy_stream_test.cc",
    ],
    deps = [
        ":chunked_stream",
        ":zero_copy_stream",
        "//upb:base",
        "//upb:mem",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
