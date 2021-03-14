load("@rules_cc//cc:defs.bzl", "cc_proto_library")
load("@rules_java//java:defs.bzl", "java_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "benchmarks_proto",
    srcs = [
        "benchmarks.proto",
    ],
    strip_import_prefix = "/benchmarks",
    visibility = [
        "//benchmarks:__subpackages__",
    ],
)

cc_proto_library(
    name = "benchmarks_cc_proto",
    visibility = [
        "//benchmarks:__subpackages__",
    ],
    deps = [
        "benchmarks_proto",
    ],
)

java_proto_library(
    name = "benchmarks_java_proto",
    visibility = [
        "//benchmarks:__subpackages__",
    ],
    deps = [
        "benchmarks_proto",
    ],
)

proto_library(
    name = "google_size_proto",
    srcs = [
        "google_size.proto",
    ],
    strip_import_prefix = "/benchmarks",
    visibility = [
        "//benchmarks:__subpackages__",
    ],
)

cc_proto_library(
    name = "google_size_cc_proto",
    visibility = [
        "//benchmarks:__subpackages__",
    ],
    deps = [
        "google_size_proto",
    ],
)

java_proto_library(
    name = "google_size_java_proto",
    visibility = [
        "//benchmarks:__subpackages__",
    ],
    deps = [
        "google_size_proto",
    ],
)
