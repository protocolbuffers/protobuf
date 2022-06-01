load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_java//java:defs.bzl", "java_proto_library")
load("@rules_pkg//:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_proto//proto:defs.bzl", "proto_library")

filegroup(
    name = "datasets",
    srcs = [],
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
)

proto_library(
    name = "benchmark_message4_proto",
    srcs = [
        "benchmark_message4.proto",
        "benchmark_message4_1.proto",
        "benchmark_message4_2.proto",
        "benchmark_message4_3.proto",
    ],
    strip_import_prefix = "/benchmarks",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
)

cc_proto_library(
    name = "benchmark_message4_cc_proto",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
    deps = [
        "benchmark_message4_proto",
    ],
)

java_proto_library(
    name = "benchmark_message4_java_proto",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
    deps = [
        "benchmark_message4_proto",
    ],
)

pkg_files(
    name = "dist_files",
    srcs = glob(["*"]),
    strip_prefix = strip_prefix.from_root(""),
    visibility = ["//benchmarks:__pkg__"],
)
