load("@rules_cc//cc:defs.bzl", "cc_binary")

cc_binary(
    name = "cpp",
    srcs = [
        "cpp_benchmark.cc",
    ],
    tags = ["benchmark"],
    deps = [
        "//:protobuf",
        "//benchmarks:benchmarks_cc_proto",
        "//benchmarks/datasets:cc_protos",
        "@com_github_google_benchmark//:benchmark_main",
    ],
)
