# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("//python:py_extension.bzl", "py_extension")
load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag")
load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

# begin:github_only
load("@rules_pkg//:mappings.bzl", "pkg_files")
load("//python:build_targets.bzl", "build_targets")
build_targets(name = "python")
# end:github_only

licenses(["notice"])

package(
    # begin:google_only
#     default_applicable_licenses = ["//upb:license"],
    # end:google_only
    default_visibility = ["//python/dist:__pkg__"],
)

LIMITED_API_FLAG_SELECT = {
    ":limited_api_3.8": ["-DPy_LIMITED_API=0x03080000"],
    ":limited_api_3.10": ["-DPy_LIMITED_API=0x030a0000"],
    "//conditions:default": [],
}

bool_flag(
    name = "limited_api",
    build_setting_default = True,
)

string_flag(
    name = "python_version",
    build_setting_default = "system",
    values = [
        "system",
        "38",
        "39",
        "310",
        "311",
    ],
)

config_setting(
    name = "limited_api_3.8",
    flag_values = {
        ":limited_api": "True",
        ":python_version": "38",
    },
)

config_setting(
    name = "full_api_3.8_win32",
    flag_values = {
        ":limited_api": "False",
        ":python_version": "38",
    },
    values = {"cpu": "win32"},
)

config_setting(
    name = "full_api_3.8_win64",
    flag_values = {
        ":limited_api": "False",
        ":python_version": "38",
    },
    values = {"cpu": "win64"},
)

selects.config_setting_group(
    name = "full_api_3.8",
    match_any = [
        ":full_api_3.8_win32",
        ":full_api_3.8_win64",
    ],
)

config_setting(
    name = "full_api_3.9_win32",
    flag_values = {
        ":limited_api": "False",
        ":python_version": "39",
    },
    values = {"cpu": "win32"},
)

config_setting(
    name = "full_api_3.9_win64",
    flag_values = {
        ":limited_api": "False",
        ":python_version": "39",
    },
    values = {"cpu": "win64"},
)

selects.config_setting_group(
    name = "full_api_3.9",
    match_any = [
        "full_api_3.9_win32",
        ":full_api_3.9_win64",
    ],
)

config_setting(
    name = "limited_api_3.10_win32",
    flag_values = {
        ":limited_api": "True",
        ":python_version": "310",
    },
    values = {"cpu": "win32"},
)

config_setting(
    name = "limited_api_3.10_win64",
    flag_values = {
        ":limited_api": "True",
        ":python_version": "310",
    },
    values = {"cpu": "win64"},
)

selects.config_setting_group(
    name = "limited_api_3.10",
    match_any = [
        ":limited_api_3.10_win32",
        ":limited_api_3.10_win64",
    ],
)

# begin:github_only
_message_target_compatible_with = {
   "@platforms//os:windows": ["@platforms//:incompatible"],
   "@system_python//:none": ["@platforms//:incompatible"],
   "@system_python//:unsupported": ["@platforms//:incompatible"],
   "//conditions:default": [],
}

# end:github_only
# begin:google_only
# _message_target_compatible_with = {
#     "@platforms//os:windows": ["@platforms//:incompatible"],
#     "//conditions:default": [],
# }
# end:google_only

filegroup(
    name = "message_srcs",
    srcs = [
        "convert.c",
        "convert.h",
        "descriptor.c",
        "descriptor.h",
        "descriptor_containers.c",
        "descriptor_containers.h",
        "descriptor_pool.c",
        "descriptor_pool.h",
        "extension_dict.c",
        "extension_dict.h",
        "map.c",
        "map.h",
        "message.c",
        "message.h",
        "protobuf.c",
        "protobuf.h",
        "python_api.h",
        "repeated.c",
        "repeated.h",
        "unknown_fields.c",
        "unknown_fields.h",
    ],
    # begin:google_only
#     compatible_with = ["//buildenv/target:non_prod"],
    # end:google_only
)

py_extension(
    name = "_message",
    srcs = [":message_srcs"],
    copts = UPB_DEFAULT_COPTS + select(LIMITED_API_FLAG_SELECT) + [
        # The Python API requires patterns that are ISO C incompatible, like
        # casts between function pointers and object pointers.
        "-Wno-pedantic",
    ],
    target_compatible_with = select(_message_target_compatible_with),
    deps = [
        "//upb:descriptor_upb_proto_reflection",
        "//upb:eps_copy_input_stream",
        "//upb:hash",
        "//upb:message",
        "//upb:message_copy",
        "//upb:port",
        "//upb:reflection",
        "//upb:text",
        "//upb:wire_reader",
        "//upb:wire_types",
        "//upb/util:compare",
        "//upb/util:def_to_proto",
        "//upb/util:required_fields",
        "@utf8_range",
    ],
)
