load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_java//java:defs.bzl", "java_proto_library")
load("@rules_pkg//:mappings.bzl", "pkg_files", "strip_prefix")
load("@rules_proto//proto:defs.bzl", "proto_library")

filegroup(
    name = "datasets",
    srcs = [
        "dataset.google_message2.pb",
    ],
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
)

proto_library(
    name = "benchmark_message2_proto",
    srcs = [
        "benchmark_message2.proto",
    ],
    strip_import_prefix = "/benchmarks",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
)

cc_proto_library(
    name = "benchmark_message2_cc_proto",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
    deps = [
        "benchmark_message2_proto",
    ],
)

java_proto_library(
    name = "benchmark_message2_java_proto",
    visibility = [
        "//benchmarks/datasets:__pkg__",
    ],
    deps = [
        "benchmark_message2_proto",
    ],
)

pkg_files(
    name = "dist_files",
    srcs = glob(["*"]),
    strip_prefix = strip_prefix.from_root(""),
    visibility = ["//benchmarks:__pkg__"],
)
