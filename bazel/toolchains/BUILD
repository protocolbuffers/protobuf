load("@bazel_skylib//:bzl_library.bzl", "bzl_library")

package(
    default_applicable_licenses = ["//:license"],
)

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
    name = "for_bazel_tests",
    testonly = True,
    srcs = [
        "BUILD",
        "proto_lang_toolchain_bzl",
        "proto_toolchain_bzl",
    ],
    visibility = [
        "//bazel:__pkg__",
    ],
)

alias(
    name = "prefer_prebuilt_protoc",
    actual = "//bazel/flags:prefer_prebuilt_protoc",
)
