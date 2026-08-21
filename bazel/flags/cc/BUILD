load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")
load("//bazel/private:compat_flag.bzl", "compat_string_list_flag")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

exports_files(["BUILD"])

label_flag(
    name = "proto_toolchain_for_cc",
    build_setting_default = "@bazel_tools//tools/proto:cc_toolchain",
)

alias(
    name = "protocopt",
    actual = "//bazel/flags:protocopt",
    deprecation = "Use //bazel/flags:protocopt instead.",
)

compat_string_list_flag(
    name = "cc_proto_library_header_suffixes",
    build_setting_default = [".pb.h"],
    fragment_field = "cc_proto_library_header_suffixes",
    scope = "universal",
)

compat_string_list_flag(
    name = "cc_proto_library_source_suffixes",
    build_setting_default = [".pb.cc"],
    fragment_field = "cc_proto_library_source_suffixes",
    scope = "universal",
)
