licenses(["unencumbered"])

exports_files(["LICENSE"])

cc_library(
    name = "wyhash",
    hdrs = ["wyhash.h"],
    visibility = ["//:__pkg__"],
)

filegroup(
    name = "cmake_files",
    srcs = glob([
        "**/*",
    ]),
    visibility = ["//cmake:__pkg__"],
)
