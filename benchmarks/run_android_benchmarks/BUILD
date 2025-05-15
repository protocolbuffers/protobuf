load("//tools/build_defs/go:go_binary.bzl", "go_binary")
load("//tools/build_defs/go:go_library.bzl", "go_library")
load("//tools/build_defs/go:go_test.bzl", "go_test")

package(default_applicable_licenses = ["//:license"])

go_binary(
    name = "run_android_benchmarks",
    srcs = ["run_android_benchmarks.go"],
    deps = [
        ":benchmarks",
        ":environment",
        ":output",
        ":parsebenchy",
        "//base/go:flag",
        "//base/go:google",
    ],
)

go_library(
    name = "parsebenchy",
    srcs = ["parsebenchy.go"],
)

go_test(
    name = "parsebenchy_test",
    srcs = ["parsebenchy_test.go"],
    library = ":parsebenchy",
    deps = ["//third_party/golang/cmp"],
)

go_library(
    name = "benchmarks",
    srcs = ["benchmarks.go"],
)

go_library(
    name = "output",
    srcs = ["output.go"],
    deps = [":benchmarks"],
)

go_library(
    name = "environment",
    srcs = ["environment.go"],
)
