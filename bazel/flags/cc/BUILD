load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_list_flag")
load("@rules_shell//shell:sh_binary.bzl", "sh_binary")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

exports_files(["BUILD"])

label_flag(
    name = "proto_toolchain_for_cc",
    build_setting_default = "@bazel_tools//tools/proto:cc_toolchain",
)

string_list_flag(
    name = "protocopt",
    build_setting_default = [],
    scope = "universal",
)

string_list_flag(
    name = "cc_proto_library_header_suffixes",
    build_setting_default = [".pb.h"],
    scope = "universal",
)

string_list_flag(
    name = "cc_proto_library_source_suffixes",
    build_setting_default = [".pb.cc"],
    scope = "universal",
)
