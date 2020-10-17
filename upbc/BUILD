load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)

licenses(["notice"])

cc_library(
    name = "upbc_generator",
    srcs = [
        "generator.cc",
        "message_layout.cc",
        "message_layout.h",
    ],
    hdrs = ["generator.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_binary(
    name = "protoc-gen-upb",
    srcs = ["main.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":upbc_generator",
        "@com_google_protobuf//:protoc_lib",
    ],
)
