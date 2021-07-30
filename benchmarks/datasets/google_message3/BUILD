load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_java//java:defs.bzl", "java_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

filegroup(
    name = "datasets",
    srcs = [],
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
)

proto_library(
    name = "benchmark_message3_proto",
    srcs = [
        "benchmark_message3.proto",
        "benchmark_message3_1.proto",
        "benchmark_message3_2.proto",
        "benchmark_message3_3.proto",
        "benchmark_message3_4.proto",
        "benchmark_message3_5.proto",
        "benchmark_message3_6.proto",
        "benchmark_message3_7.proto",
        "benchmark_message3_8.proto",
    ],
    strip_import_prefix = "/benchmarks",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
)

cc_proto_library(
    name = "benchmark_message3_cc_proto",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
    deps = [
        "benchmark_message3_proto",
    ],
)

java_proto_library(
    name = "benchmark_message3_java_proto",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
    deps = [
        "benchmark_message3_proto",
    ],
)
