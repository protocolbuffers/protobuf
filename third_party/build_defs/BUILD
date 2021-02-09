load("//tools/build_defs/testing:bzl_library.bzl", "bzl_library")

licenses(["notice"])

bzl_library(
    name = "kt_jvm_proto_library_helper",
    srcs = ["kt_jvm_proto_library.internal.bzl"],
    visibility = ["//visibility:private"],
    deps = [
        "//tools/build_defs/kotlin/release/rules:android_jvm_compile_bzl",
        "//tools/build_defs/kotlin/release/toolchains/kotlin_jvm:toolchain_bzl",
    ],
)

bzl_library(
    name = "kt_jvm_proto_library",
    srcs = ["kt_jvm_proto_library.bzl"],
    visibility = ["//visibility:private"],
    deps = [":kt_jvm_proto_library_helper"],
)
