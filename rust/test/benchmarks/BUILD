# Benchmarks for Rust Protobuf.
load(
    "@rules_rust//rust:defs.bzl",
    "rust_library",
)
load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load(
    "//protos/bazel:upb_cc_proto_library.bzl",
    "upb_cc_proto_library",
)
load(
    "//rust:defs.bzl",
    "rust_cc_proto_library",
    "rust_upb_proto_library",
)

proto_library(
    name = "bench_data_proto",
    srcs = ["bench_data.proto"],
)

cc_proto_library(
    name = "bench_data_cc_proto",
    deps = [":bench_data_proto"],
)

rust_cc_proto_library(
    name = "bench_data_cpp_rust_proto",
    deps = [":bench_data_proto"],
)

rust_upb_proto_library(
    name = "bench_data_upb_rust_proto",
    deps = [":bench_data_proto"],
)

upb_cc_proto_library(
    name = "bench_data_upb_cc_proto",
    deps = [":bench_data_proto"],
)

cc_test(
    name = "rust_protobuf_benchmarks",
    testonly = True,
    srcs = ["rust_protobuf_benchmarks.cc"],
    deps = [
        ":bench_data_cc_proto",
        ":bench_data_upb_cc_proto",
        ":benchmarks",
        ":proto_benchmarks_cpp",  # build_cleaner: keep
        ":proto_benchmarks_upb",  # build_cleaner: keep
        "//protos",
        "//protos:repeated_field",
        "//src/google/protobuf:protobuf_lite",
        "//testing/base/public:gunit",
        "//third_party/benchmark",
        "@com_google_absl//absl/log:absl_check",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

rust_library(
    name = "proto_benchmarks_cpp",
    srcs = ["proto_benchmarks.rs"],
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    rustc_flags = [
        "--cfg=bench_cpp",
    ],
    deps = [":bench_data_cpp_rust_proto"],
)

rust_library(
    name = "proto_benchmarks_upb",
    srcs = ["proto_benchmarks.rs"],
    proc_macro_deps = [
        "@crate_index//:paste",
    ],
    rustc_flags = [
        "--cfg=bench_upb",
    ],
    deps = [":bench_data_upb_rust_proto"],
)

rust_library(
    name = "benchmarks",
    srcs = ["benchmarks.rs"],
    deps = [
        ":benchmark_thunks",  #build_cleaner: keep
    ],
)

cc_library(
    name = "benchmark_thunks",
    srcs = ["benchmark_thunks.cc"],
    deps = [":bench_data_cc_proto"],
)
