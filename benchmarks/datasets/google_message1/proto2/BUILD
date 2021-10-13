load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_java//java:defs.bzl", "java_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

filegroup(
    name = "datasets",
    srcs = [
        "dataset.google_message1_proto2.pb",
    ],
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
)

proto_library(
    name = "benchmark_message1_proto2_proto",
    srcs = [
        "benchmark_message1_proto2.proto",
    ],
    strip_import_prefix = "/benchmarks",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
)

cc_proto_library(
    name = "benchmark_message1_proto2_cc_proto",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
    deps = [
        "benchmark_message1_proto2_proto",
    ],
)

java_proto_library(
    name = "benchmark_message1_proto2_java_proto",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
    deps = [
        "benchmark_message1_proto2_proto",
    ],
)
