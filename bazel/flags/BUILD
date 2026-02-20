load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag", "string_list_flag")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

bzl_library(
    name = "flags_bzl",
    srcs = ["flags.bzl"],
    deps = [
        "@bazel_skylib//rules:common_settings",
    ],
)

filegroup(
    name = "for_bazel_tests",
    testonly = True,
    srcs = [
        "BUILD",
        "flags.bzl",
        "//bazel/flags/cc:BUILD",
        "//bazel/flags/java:BUILD",
    ],
)

bool_flag(
    name = "experimental_proto_descriptor_sets_include_source_info",
    build_setting_default = False,
    scope = "universal",
)

label_flag(
    name = "proto_compiler",
    build_setting_default = "@bazel_tools//tools/proto:protoc",
)

# When set to true, we will use a prebuilt protoc binary from GitHub if it's available.
# This is now enabled by default.
bool_flag(
    name = "prefer_prebuilt_protoc",
    build_setting_default = True,
    visibility = ["//bazel/toolchains:__pkg__"],
)

config_setting(
    name = "prefer_prebuilt_protoc.flag_set",
    flag_values = {":prefer_prebuilt_protoc": "true"},
    visibility = [
        "//bazel/private/oss/toolchains/prebuilt:__pkg__",
        # Needed by cc_toolchain to switch between minimal and full protoc
        "//:__pkg__",
    ],
)

# The public API users set to disable the validation action failing.
bool_flag(
    name = "allow_nonstandard_protoc",
    build_setting_default = False,
)

config_setting(
    name = "allow_nonstandard_protoc.flag_set",
    flag_values = {":allow_nonstandard_protoc": "true"},
    visibility = ["//bazel/private/oss/toolchains/prebuilt:__pkg__"],
)

# TODO: deprecate this flag.
string_flag(
    name = "strict_proto_deps",
    build_setting_default = "error",
    scope = "universal",
    values = [
        "off",
        "OFF",
        "warn",
        "WARN",
        "error",
        "ERROR",
        "strict",
        "STRICT",
        "default",
        "DEFAULT",
    ],
)

# TODO: deprecate this flag.
string_flag(
    name = "strict_public_imports",
    build_setting_default = "off",
    scope = "universal",
    values = [
        "off",
        "OFF",
        "warn",
        "WARN",
        "error",
        "ERROR",
        "strict",
        "STRICT",
        "default",
        "DEFAULT",
    ],
)
