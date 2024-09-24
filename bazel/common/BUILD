load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(default_applicable_licenses = ["//:license"])

bzl_library(
    name = "proto_common_bzl",
    srcs = [
        "proto_common.bzl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":proto_lang_toolchain_info_bzl",
        "//bazel/private:toolchain_helpers_bzl",
        "@proto_bazel_features//:features",
    ],
)

bzl_library(
    name = "proto_info_bzl",
    srcs = [
        "proto_info.bzl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/private:native_bzl",
    ],
)

bzl_library(
    name = "proto_lang_toolchain_info_bzl",
    srcs = [
        "proto_lang_toolchain_info.bzl",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//bazel/private:native_bzl",
    ],
)

filegroup(
    name = "bazel_osx_p4deps",
    srcs = glob(["**"]) + ["@proto_bazel_features//:features"],
    visibility = [
        "//bazel:__pkg__",
    ],
)
