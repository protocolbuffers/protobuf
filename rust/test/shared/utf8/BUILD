load("@rules_rust//rust:defs.bzl", "rust_test")
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//rust:defs.bzl", "rust_cc_proto_library", "rust_upb_proto_library")

licenses(["notice"])

rust_test(
    name = "utf8_cpp_test",
    srcs = ["utf8_test.rs"],
    aliases = {
        "//rust:protobuf_cpp_export": "protobuf",
    },
    deps = [
        ":feature_verify_cpp_rust_proto",
        ":no_features_proto2_cpp_rust_proto",
        ":no_features_proto3_cpp_rust_proto",
        "//rust:protobuf_cpp_export",
        "@crate_index//:googletest",
    ],
)

rust_test(
    name = "utf8_upb_test",
    srcs = ["utf8_test.rs"],
    aliases = {
        "//rust:protobuf_upb_export": "protobuf",
    },
    deps = [
        ":feature_verify_upb_rust_proto",
        ":no_features_proto2_upb_rust_proto",
        ":no_features_proto3_upb_rust_proto",
        "//rust:protobuf_upb_export",
        "@crate_index//:googletest",
    ],
)

cc_test(
    name = "utf8_pure_cc_test",
    srcs = ["utf8_test.cc"],
    deps = [
        ":feature_verify_cc_proto",
        ":no_features_proto2_cc_proto",
        ":no_features_proto3_cc_proto",
        "@abseil-cpp//absl/strings:string_view",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

proto_library(
    name = "no_features_proto2_proto",
    srcs = ["no_features_proto2.proto"],
)

proto_library(
    name = "no_features_proto3_proto",
    srcs = ["no_features_proto3.proto"],
)

proto_library(
    name = "feature_verify_proto",
    srcs = ["feature_verify.proto"],
)

cc_proto_library(
    name = "no_features_proto2_cc_proto",
    deps = [":no_features_proto2_proto"],
)

cc_proto_library(
    name = "no_features_proto3_cc_proto",
    deps = [":no_features_proto3_proto"],
)

cc_proto_library(
    name = "feature_verify_cc_proto",
    deps = [":feature_verify_proto"],
)

rust_upb_proto_library(
    name = "no_features_proto2_upb_rust_proto",
    deps = [":no_features_proto2_proto"],
)

rust_upb_proto_library(
    name = "no_features_proto3_upb_rust_proto",
    deps = [":no_features_proto3_proto"],
)

rust_upb_proto_library(
    name = "feature_verify_upb_rust_proto",
    deps = [":feature_verify_proto"],
)

rust_cc_proto_library(
    name = "no_features_proto2_cpp_rust_proto",
    deps = [":no_features_proto2_proto"],
)

rust_cc_proto_library(
    name = "no_features_proto3_cpp_rust_proto",
    deps = [":no_features_proto3_proto"],
)

rust_cc_proto_library(
    name = "feature_verify_cpp_rust_proto",
    deps = [":feature_verify_proto"],
)
