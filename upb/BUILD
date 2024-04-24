# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("@rules_python//python:defs.bzl", "py_binary")
load("//bazel/private:upb_proto_library_internal/copts.bzl", "upb_proto_library_copts")
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

# begin:google_only
# load("//tools/build_defs/kotlin/native:rules.bzl", "kt_native_interop_hint")
# load("//tools/build_defs/license:license.bzl", "license")
# end:google_only

# begin:github_only
load(
    "//upb/bazel:amalgamation.bzl",
    "upb_amalgamation",
)
# end:github_only

# begin:google_only
# package(default_applicable_licenses = [":license"])
#
# license(
#     name = "license",
#     package_name = "upb",
# )
# end:google_only

licenses(["notice"])

exports_files(["LICENSE"])

exports_files(
    [
        "BUILD",
        "WORKSPACE",
    ],
    visibility = ["//upb/cmake:__pkg__"],
)

config_setting(
    name = "windows",
    constraint_values = ["@platforms//os:windows"],
    visibility = ["//visibility:public"],
)

bool_flag(
    name = "fasttable_enabled",
    build_setting_default = False,
    visibility = ["//visibility:public"],
)

config_setting(
    name = "fasttable_enabled_setting",
    flag_values = {"//upb:fasttable_enabled": "true"},
    visibility = ["//visibility:public"],
)

upb_proto_library_copts(
    name = "upb_proto_library_copts__for_generated_code_only_do_not_use",
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
)

# Please update copy.bara.sky target = ":friends" if
# you make changes to this list.
package_group(
    name = "friends",
    packages = ["//..."],
)

package_group(
    name = "friend_generators",
    packages = ["//..."],
)

# Common support routines used by generated code.  This library has no
# implementation, but depends on :upb and exposes a few more hdrs.
#
# This is public only because we have no way of visibility-limiting it to
# upb_c_proto_library() only. This interface is not stable and by using it you
# give up any backward compatibility guarantees.
cc_library(
    name = "generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = ["generated_code_support.h"],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        ":port_inc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":mem",
        ":message",
        ":mini_descriptor",
        ":mini_table",
        ":wire",
        "//upb/message:internal",
    ],
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        ":port_inc",
    ],
    visibility = ["//visibility:public"],
)

# Aliases ######################################################################
# TODO: Remove these.

alias(
    name = "base",
    actual = "//upb/base",
    visibility = ["//visibility:public"],
)

alias(
    name = "descriptor_upb_proto",
    actual = "//upb/reflection:descriptor_upb_proto",
    visibility = ["//visibility:public"],
)

alias(
    name = "descriptor_upb_minitable_proto",
    actual = "//upb/reflection:descriptor_upb_minitable_proto",
    visibility = ["//visibility:public"],
)

alias(
    name = "descriptor_upb_proto_reflection",
    actual = "//upb/reflection:descriptor_upb_proto_reflection",
    visibility = ["//visibility:public"],
)

alias(
    name = "generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    actual = "//upb/reflection:generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    visibility = ["//visibility:public"],
)

alias(
    name = "json",
    actual = "//upb/json",
    visibility = ["//visibility:public"],
)

alias(
    name = "mem",
    actual = "//upb/mem",
    visibility = ["//visibility:public"],
)

alias(
    name = "message",
    actual = "//upb/message",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_compare",
    actual = "//upb/message:compare",
    visibility = ["//upb:friends"],
)

alias(
    name = "message_copy",
    actual = "//upb/message:copy",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_promote",
    actual = "//upb/message:promote",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_split64",
    actual = "//upb/message:split64",
    visibility = ["//visibility:public"],
)

alias(
    name = "mini_descriptor",
    actual = "//upb/mini_descriptor",
    visibility = ["//visibility:public"],
)

alias(
    name = "mini_table",
    actual = "//upb/mini_table",
    visibility = ["//upb:friends"],
)

alias(
    name = "mini_table_compat",
    actual = "//upb/mini_table:compat",
    visibility = ["//upb:friends"],
)

alias(
    name = "port",
    actual = "//upb/port",
    visibility = ["//visibility:public"],
)

alias(
    name = "port_inc",
    actual = "//upb/port:inc",
    visibility = ["//visibility:public"],
)

alias(
    name = "reflection",
    actual = "//upb/reflection",
    visibility = ["//visibility:public"],
)

alias(
    name = "text",
    actual = "//upb/text",
    visibility = ["//visibility:public"],
)

alias(
    name = "wire",
    actual = "//upb/wire",
    visibility = ["//visibility:public"],
)

alias(
    name = "wire_reader",
    actual = "//upb/wire:reader",
    visibility = ["//visibility:public"],
)

alias(
    name = "eps_copy_input_stream",
    actual = "//upb/wire:eps_copy_input_stream",
    visibility = ["//visibility:public"],
)

# Amalgamation #################################################################

# begin:github_only

upb_amalgamation(
    name = "gen_amalgamation",
    outs = [
        "upb.c",
        "upb.h",
    ],
    libs = [
        ":base",
        ":descriptor_upb_minitable_proto",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":mem",
        ":message",
        ":message_compare",
        ":message_copy",
        ":mini_descriptor",
        ":mini_table",
        ":mini_table_compat",
        ":port",
        ":reflection",
        ":wire",
        ":wire_reader",
        "//upb/base:internal",
        "//upb/hash:hash",
        "//upb/lex:lex",
        "//upb/mem:internal",
        "//upb/message:internal",
        "//upb/message:types",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table:internal",
        "//upb/reflection:internal",
    ],
    strip_import_prefix = ["src"],
)

cc_library(
    name = "amalgamation",
    srcs = ["upb.c"],
    hdrs = ["upb.h"],
    copts = UPB_DEFAULT_COPTS,
    deps = ["//third_party/utf8_range"],
)

upb_amalgamation(
    name = "gen_php_amalgamation",
    outs = [
        "php-upb.c",
        "php-upb.h",
    ],
    libs = [
        ":base",
        ":descriptor_upb_minitable_proto",
        ":descriptor_upb_proto_reflection",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":json",
        ":mem",
        ":message",
        ":message_compare",
        ":message_copy",
        ":mini_descriptor",
        ":mini_table",
        ":mini_table_compat",
        ":port",
        ":reflection",
        ":wire",
        ":wire_reader",
        "//upb/base:internal",
        "//upb/hash:hash",
        "//upb/lex:lex",
        "//upb/mem:internal",
        "//upb/message:internal",
        "//upb/message:types",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table:internal",
        "//upb/reflection:internal",
    ],
    prefix = "php-",
    strip_import_prefix = ["src"],
    visibility = ["@com_google_protobuf//php:__subpackages__"],
)

cc_library(
    name = "php_amalgamation",
    srcs = ["php-upb.c"],
    hdrs = ["php-upb.h"],
    copts = UPB_DEFAULT_COPTS,
    deps = ["//third_party/utf8_range"],
)

upb_amalgamation(
    name = "gen_ruby_amalgamation",
    outs = [
        "ruby-upb.c",
        "ruby-upb.h",
    ],
    libs = [
        ":base",
        ":descriptor_upb_minitable_proto",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":json",
        ":mem",
        ":message",
        ":message_compare",
        ":message_copy",
        ":mini_descriptor",
        ":mini_table",
        ":mini_table_compat",
        ":port",
        ":reflection",
        ":wire",
        ":wire_reader",
        "//upb/base:internal",
        "//upb/hash:hash",
        "//upb/lex:lex",
        "//upb/mem:internal",
        "//upb/message:internal",
        "//upb/message:types",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table:internal",
        "//upb/reflection:internal",
    ],
    prefix = "ruby-",
    strip_import_prefix = ["src"],
    visibility = ["@com_google_protobuf//ruby:__subpackages__"],
)

cc_library(
    name = "ruby_amalgamation",
    srcs = ["ruby-upb.c"],
    hdrs = ["ruby-upb.h"],
    copts = UPB_DEFAULT_COPTS,
    deps = ["//third_party/utf8_range"],
)

exports_files(
    [
        "third_party/lunit/console.lua",
        "third_party/lunit/lunit.lua",
    ],
    visibility = ["//lua:__pkg__"],
)

filegroup(
    name = "source_files",
    srcs = glob(
        [
            "**/*.h",
        ],
        exclude = [
            "**/conformance_upb.c",
            "reflection/stage0/**/*",
        ],
    ),
    visibility = [
        "//python/dist:__pkg__",
        "//upb/cmake:__pkg__",
    ],
)

filegroup(
    name = "test_protos",
    srcs = [
        "//src/google/protobuf:test_messages_proto2.proto",
        "//src/google/protobuf:test_messages_proto3.proto",
        "//src/google/protobuf:well_known_type_protos",
        "//upb/json:test_protos",
        "//upb/message:test_protos",
        "//upb/test:test_protos",
        "//upb/util:test_protos",
    ],
    visibility = ["//pkg:__pkg__"],
)

filegroup(
    name = "test_srcs",
    srcs = [
        "//upb/json:test_srcs",
        "//upb/mem:test_srcs",
        "//upb/message:test_srcs",
        "//upb/test:test_srcs",
        "//upb/util:test_srcs",
        "//upb/wire:test_srcs",
    ],
    visibility = ["//pkg:__pkg__"],
)

filegroup(
    name = "test_util",
    srcs = [
        "//upb/test:test_util",
        "//upb/util:test_util",
    ],
    visibility = ["//pkg:__pkg__"],
)
# end:github_only

# begin:google_only
#
# py_binary(
#     name = "update_check_runs",
#     srcs = ["update_check_runs.py"],
#     main = "update_check_runs.py",
#     deps = [
#         "//third_party/py/absl:app",
#         "//third_party/py/absl/flags",
#     ],
# )
#
# kt_native_interop_hint(
#     name = "upb_kotlin_native_hint",
#     compatible_with = ["//buildenv/target:non_prod"],
#     headers_to_exclude = glob([
#         "**/*.hpp",
#     ]),
#     kotlin_package = "upb",
#     no_string_conversion = [
#         "_upb_MiniTable_Build",
#         "upb_Decode",
#         "upb_StringView_FromDataAndSize",
#     ],
#     strict_enums = [
#         "upb_CType",
#         "upb_DecodeStatus",
#         "upb_EncodeStatus",
#         "upb_FieldType",
#         "upb_FindUnknown_Status",
#         "upb_GetExtension_Status",
#         "upb_GetExtensionAsBytes_Status",
#         "upb_Label",
#         "upb_MapInsertStatus",
#         "upb_UnknownToMessage_Status",
#         "upb_WireType",
#     ],
#     visibility = [
#         "//third_party/kotlin/protobuf:__subpackages__",
#         "//upb:__subpackages__",
#     ],
# )
#
# end:google_only
