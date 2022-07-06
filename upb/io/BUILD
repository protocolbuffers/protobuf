cc_library(
    name = "zero_copy_stream",
    hdrs = [
        "zero_copy_input_stream.h",
        "zero_copy_output_stream.h",
    ],
    visibility = ["//upb/io:__pkg__"],
    deps = [
        "//:upb",
        "//:port",
    ],
)

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
    visibility = ["//upb/io:__pkg__"],
    deps = [
        ":zero_copy_stream",
        "//:upb",
        "//:port",
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
        "//:upb",
        "@com_google_googletest//:gtest_main",
    ],
)
