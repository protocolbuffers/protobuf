# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Google LLC nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# begin:github_only
load("//python:py_extension.bzl", "py_extension")
# end:github_only

load("@bazel_skylib//lib:selects.bzl", "selects")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "string_flag")
load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

# begin:github_only
load("@rules_pkg//:mappings.bzl", "pkg_files")
# end:github_only

licenses(["notice"])

package(
    default_visibility = ["//python/dist:__pkg__"],
)

LIMITED_API_FLAG_SELECT = {
    ":limited_api_3.7": ["-DPy_LIMITED_API=0x03070000"],
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
        "37",
        "38",
        "39",
        "310",
    ],
)

config_setting(
    name = "limited_api_3.7",
    flag_values = {
        ":limited_api": "True",
        ":python_version": "37",
    },
)

config_setting(
    name = "full_api_3.7_win32",
    flag_values = {
        ":limited_api": "False",
        ":python_version": "37",
    },
    values = {"cpu": "win32"},
)

config_setting(
    name = "full_api_3.7_win64",
    flag_values = {
        ":limited_api": "False",
        ":python_version": "37",
    },
    values = {"cpu": "win64"},
)

selects.config_setting_group(
    name = "full_api_3.7",
    match_any = [
        ":full_api_3.7_win32",
        ":full_api_3.7_win64",
    ],
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

pkg_files(
   name = "message_extension",
   srcs = [":message_srcs"],
   prefix = "python/",
)
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
        "//:collections",
        "//:descriptor_upb_proto_reflection",
        "//:eps_copy_input_stream",
        "//:hash",
        "//:port",
        "//:reflection",
        "//:textformat",
        "//:upb",
        "//:wire",
        "//:wire_reader",
        "//:wire_types",
        "//upb/util:compare",
        "//upb/util:def_to_proto",
        "//upb/util:required_fields",
    ],
)
