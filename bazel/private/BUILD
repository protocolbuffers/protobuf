load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load(":native_bool_flag.bzl", "native_bool_flag")

package(default_applicable_licenses = ["//:license"])

toolchain_type(
    name = "proto_toolchain_type",
    visibility = ["//visibility:public"],
)

toolchain_type(
    name = "cc_toolchain_type",
    visibility = ["//visibility:public"],
)

toolchain_type(
    name = "java_toolchain_type",
    visibility = ["//visibility:public"],
)

toolchain_type(
    name = "javalite_toolchain_type",
    visibility = ["//visibility:public"],
)

toolchain_type(
    name = "python_toolchain_type",
    visibility = ["//visibility:public"],
)

bzl_library(
    name = "native_bzl",
    srcs = [
        "native.bzl",
    ],
    visibility = ["//bazel:__subpackages__"],
)

bzl_library(
    name = "proto_info_bzl",
    srcs = ["proto_info.bzl"],
    visibility = ["//bazel:__subpackages__"],
)

bzl_library(
    name = "proto_library_rule_bzl",
    srcs = [
        "proto_library_rule.bzl",
    ],
    visibility = ["//bazel:__subpackages__"],
    deps = [
        ":toolchain_helpers_bzl",
        "//bazel/common:proto_common_bzl",
        "//bazel/common:proto_info_bzl",
        "@bazel_skylib//lib:paths",
        "@bazel_skylib//rules:common_settings",
        "@proto_bazel_features//:features",
    ],
)

bzl_library(
    name = "java_proto_library_bzl",
    srcs = [
        "java_proto_library.bzl",
        "java_proto_support.bzl",
    ],
    visibility = [
        "//bazel:__subpackages__",
    ],
    deps = [
        ":toolchain_helpers_bzl",
        "//bazel/common:proto_common_bzl",
        "//bazel/common:proto_info_bzl",
        "@rules_java//java/common",
    ],
)

bzl_library(
    name = "java_lite_proto_library_bzl",
    srcs = [
        "java_lite_proto_library.bzl",
        "java_proto_support.bzl",
    ],
    visibility = [
        "//bazel:__subpackages__",
    ],
    deps = [
        ":toolchain_helpers_bzl",
        "//bazel/common:proto_common_bzl",
        "//bazel/common:proto_info_bzl",
        "@rules_java//java/common",
    ],
)

bzl_library(
    name = "cc_proto_support_bzl",
    srcs = ["cc_proto_support.bzl"],
    visibility = ["//bazel/private:__subpackages__"],
    deps = [
        "@proto_bazel_features//:features",
        "@rules_cc//cc:find_cc_toolchain_bzl",
        "@rules_cc//cc/common",
    ],
)

bzl_library(
    name = "proto_toolchain_rule_bzl",
    srcs = [
        "proto_toolchain_rule.bzl",
    ],
    visibility = ["//bazel:__subpackages__"],
    deps = [
        ":toolchain_helpers_bzl",
        "//bazel/common:proto_common_bzl",
        "//bazel/common:proto_lang_toolchain_info_bzl",
    ],
)

bzl_library(
    name = "proto_lang_toolchain_rule_bzl",
    srcs = [
        "proto_lang_toolchain_rule.bzl",
    ],
    visibility = ["//bazel:__subpackages__"],
    deps = [
        ":toolchain_helpers_bzl",
        "//bazel/common:proto_common_bzl",
        "//bazel/common:proto_info_bzl",
        "//bazel/common:proto_lang_toolchain_info_bzl",
        "@proto_bazel_features//:features",
    ],
)

bzl_library(
    name = "java_proto_aspect_names_bzl",
    srcs = ["java_proto_aspect_names.bzl"],
    visibility = ["@rules_java//java/google/rules:__pkg__"],
)

bzl_library(
    name = "cc_proto_aspect_bzl",
    srcs = ["cc_proto_aspect.bzl"],
    visibility = [
        "//rust/bazel:__pkg__",
    ],
    deps = [
        "//bazel/private/oss:cc_proto_library_bzl",
    ],
)

bzl_library(
    name = "cc_proto_aspect_name_bzl",
    srcs = ["cc_proto_aspect_name.bzl"],
    visibility = [
    ],
)

bzl_library(
    name = "toolchain_helpers_bzl",
    srcs = [
        "toolchain_helpers.bzl",
    ],
    visibility = ["//bazel:__subpackages__"],
    deps = [
        ":native_bzl",
        "//bazel/common:proto_lang_toolchain_info_bzl",
        "//bazel/flags:flags_bzl",
    ],
)

native_bool_flag(
    name = "experimental_proto_descriptor_sets_include_source_info",
    flag = "experimental_proto_descriptor_sets_include_source_info",
    match_value = "true",
    visibility = ["//bazel:__subpackages__"],
)

native_bool_flag(
    name = "strict_proto_deps",
    flag = "strict_proto_deps",
    match_value = "off",
    result = False,
    visibility = ["//bazel:__subpackages__"],
)

native_bool_flag(
    name = "strict_public_imports",
    flag = "strict_public_imports",
    match_value = "off",
    result = False,
    visibility = ["//bazel:__subpackages__"],
)

bzl_library(
    name = "native_bool_flag_bzl",
    srcs = ["native_bool_flag.bzl"],
    visibility = ["//visibility:private"],
    deps = ["@bazel_skylib//rules:common_settings"],
)

filegroup(
    name = "for_bazel_tests",
    testonly = True,
    srcs = [
        "BUILD",
        ":java_proto_library_bzl",
        ":native_bool_flag_bzl",
        ":toolchain_helpers_bzl",
        "//bazel:for_bazel_tests",
        "//bazel/private/oss/toolchains:for_bazel_tests",
    ],
    visibility = ["//visibility:public"],
)
