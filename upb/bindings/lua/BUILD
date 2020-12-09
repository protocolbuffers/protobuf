load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)

licenses(["notice"])

cc_library(
    name = "lupb",
    srcs = [
        "def.c",
        "msg.c",
        "upb.c",
    ],
    hdrs = [
        "upb.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        "//:json",
        "//:reflection",
        "//:textformat",
        "//:upb",
        "@lua//:liblua",
    ],
)

cc_binary(
    name = "protoc-gen-lua",
    srcs = ["upbc.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protoc_lib",
    ],
)

exports_files(["upb.lua"])
