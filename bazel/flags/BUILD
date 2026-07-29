load("@bazel_skylib//:bzl_library.bzl", "bzl_library")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("//bazel/private:compat_flag.bzl", "compat_bool_flag", "compat_string_list_flag")

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

compat_bool_flag(
    name = "experimental_proto_descriptor_sets_include_source_info",
    build_setting_default = False,
    fragment_field = "experimental_proto_descriptorsets_include_source_info",
    scope = "universal",
)

label_flag(
    name = "proto_compiler",
    build_setting_default = "@bazel_tools//tools/proto:protoc",
)

compat_string_list_flag(
    name = "protocopt",
    build_setting_default = [],
    fragment_field = "experimental_protoc_opts",
    scope = "universal",
)

# When set to true, we will use a prebuilt protoc binary from GitHub if it's available.
# This is now enabled by default.
bool_flag(
    name = "prefer_prebuilt_protoc",
    build_setting_default = True,
    # See https://github.com/protocolbuffers/protobuf/issues/25561
    scope = "universal",  # propagate from host to exec configuration
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
compat_bool_flag(
    name = "strict_proto_deps",
    build_setting_default = True,
    fragment_field = "strict_proto_deps",
    scope = "universal",
    values = {
        False: [
            "off",
            "OFF",
        ],
        True: [
            "warn",
            "WARN",
            "error",
            "ERROR",
            "strict",
            "STRICT",
        ],
    },
)

# TODO: deprecate this flag.
compat_bool_flag(
    name = "strict_public_imports",
    build_setting_default = False,
    fragment_field = "strict_public_imports",
    scope = "universal",
    values = {
        False: [
            "off",
            "OFF",
        ],
        True: [
            "warn",
            "WARN",
            "error",
            "ERROR",
            "strict",
            "STRICT",
        ],
    },
)
