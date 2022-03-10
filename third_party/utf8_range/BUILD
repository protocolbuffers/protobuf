
# Pulled from: https://github.com/cyb70289/utf8

cc_library(
    name = "utf8_range",
    hdrs = ["utf8_range.h"],
    srcs = [
        "naive.c",
        "range2-neon.c",
        "range2-sse.c",
    ],
    visibility = ["//:__pkg__"],
)

filegroup(
    name = "cmake_files",
    srcs = glob(["*"]),
    visibility = ["//cmake:__pkg__"],
)
