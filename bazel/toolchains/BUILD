load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(default_applicable_licenses = ["//:license"])

bzl_library(
    name = "proto_toolchain_bzl",
    srcs = [
        "proto_toolchain.bzl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/private:proto_toolchain_rule_bzl",
        "//bazel/private:toolchain_helpers_bzl",
    ],
)

bzl_library(
    name = "proto_lang_toolchain_bzl",
    srcs = [
        "proto_lang_toolchain.bzl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/common:proto_common_bzl",
        "//bazel/private:proto_lang_toolchain_rule_bzl",
        "@proto_bazel_features//:features",
    ],
)

filegroup(
    name = "bazel_osx_p4deps",
    srcs = glob(["**"]),
    visibility = [
        "//bazel:__pkg__",
    ],
)
