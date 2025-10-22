load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")

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

# The public API users set
bool_flag(
    name = "prefer_prebuilt_protoc",
    # TODO(alexeagle): this should be True after the feature is vetted with some adoption
    build_setting_default = False,
    visibility = ["//visibility:public"],
)

config_setting(
    name = "prefer_prebuilt_protoc.flag_set",
    flag_values = {":prefer_prebuilt_protoc": "true"},
)
