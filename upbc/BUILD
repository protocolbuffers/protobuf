load(
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)

licenses(["notice"])

cc_library(
    name = "common",
    hdrs = ["common.h"],
    srcs = ["common.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        "@com_google_protobuf//:protobuf",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "protoc-gen-upb",
    srcs = [
        "protoc-gen-upb.cc",
        "message_layout.cc",
        "message_layout.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)

cc_binary(
    name = "protoc-gen-upbdefs",
    srcs = [
        "protoc-gen-upbdefs.cc",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":common",
        "@com_google_absl//absl/base:core_headers",
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/strings",
        "@com_google_protobuf//:protobuf",
        "@com_google_protobuf//:protoc_lib",
    ],
)
