# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_python//python:defs.bzl", "py_binary")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("//bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")
load("//bazel:upb_minitable_proto_library.bzl", "upb_minitable_proto_library")
load("//bazel:upb_proto_library.bzl", "upb_proto_reflection_library")
load("//bazel:upb_proto_library_internal/copts.bzl", "upb_proto_library_copts")
load(
    "//upb_generator:bootstrap_compiler.bzl",
    "bootstrap_cc_library",
    "bootstrap_upb_proto_library",
)

# begin:google_only
# load("//tools/build_defs/kotlin/native:rules.bzl", "kt_native_interop_hint")
# load("//tools/build_defs/license:license.bzl", "license")
# end:google_only

# begin:github_only
load(
    "//bazel:amalgamation.bzl",
    "upb_amalgamation",
)
# end:github_only

# begin:google_only
# package(default_applicable_licenses = ["//upb:license"])
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

cc_library(
    name = "upb",
    hdrs = [
        "upb.hpp",
    ],
    copts = UPB_DEFAULT_COPTS,
    deprecation = "use upb:base and/or upb:mem instead",
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":mem",
    ],
)

# Common support routines used by generated code.  This library has no
# implementation, but depends on :upb and exposes a few more hdrs.
#
# This is public only because we have no way of visibility-limiting it to
# upb_proto_library() only.  This interface is not stable and by using it you
# give up any backward compatibility guarantees.
cc_library(
    name = "generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = ["generated_code_support.h"],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "//upb/port:inc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":mem",
        ":message",
        ":message_accessors",
        ":message_accessors_internal",
        ":message_internal",
        ":mini_descriptor",
        ":mini_table",
        ":wire",
        ":wire_internal",
    ],
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "//upb/port:inc",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = [
        "reflection/def.h",
        "reflection/internal/def_pool.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "//upb/port:inc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":mem",
        ":mini_descriptor",
        ":reflection_internal",
    ],
)

bootstrap_upb_proto_library(
    name = "descriptor_upb_proto",
    base_dir = "reflection/",
    # TODO: Export 'net/proto2/proto/descriptor.upb.h' and remove "-layering_check".
    features = ["-layering_check"],
    google3_src_files = ["net/proto2/proto/descriptor.proto"],
    google3_src_rules = ["//net/proto2/proto:descriptor_proto_source"],
    oss_src_files = ["google/protobuf/descriptor.proto"],
    oss_src_rules = ["//:descriptor_proto_srcs"],
    oss_strip_prefix = "third_party/protobuf/github/bootstrap/src",
    proto_lib_deps = ["//:descriptor_proto"],
    visibility = ["//visibility:public"],
)

upb_proto_reflection_library(
    name = "descriptor_upb_proto_reflection",
    visibility = ["//visibility:public"],
    deps = ["//:descriptor_proto"],
)

upb_minitable_proto_library(
    name = "descriptor_upb_minitable_proto",
    visibility = [
        "//upb:__subpackages__",
        # begin:github_only
        "//python:__subpackages__",
        # end:github_only
    ],
    deps = ["//:descriptor_proto"],
)

# TODO: Once we can delete the deprecated forwarding headers
# (= everything in upb/) we can move this build target down into reflection/
bootstrap_cc_library(
    name = "reflection",
    hdrs = [
        "reflection/def.h",
        "reflection/def.hpp",
        "reflection/message.h",
        "reflection/message.hpp",
    ],
    bootstrap_deps = [":reflection_internal"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":mem",
        ":message",
        ":message_types",
        ":message_value",
        ":port",
    ],
)

bootstrap_cc_library(
    name = "reflection_internal",
    srcs = [
        "reflection/def_pool.c",
        "reflection/def_type.c",
        "reflection/desc_state.c",
        "reflection/enum_def.c",
        "reflection/enum_reserved_range.c",
        "reflection/enum_value_def.c",
        "reflection/extension_range.c",
        "reflection/field_def.c",
        "reflection/file_def.c",
        "reflection/internal/def_builder.c",
        "reflection/internal/def_builder.h",
        "reflection/internal/strdup2.c",
        "reflection/internal/strdup2.h",
        "reflection/message.c",
        "reflection/message_def.c",
        "reflection/message_reserved_range.c",
        "reflection/method_def.c",
        "reflection/oneof_def.c",
        "reflection/service_def.c",
    ],
    hdrs = [
        "reflection/common.h",
        "reflection/def.h",
        "reflection/def.hpp",
        "reflection/def_pool.h",
        "reflection/def_type.h",
        "reflection/enum_def.h",
        "reflection/enum_reserved_range.h",
        "reflection/enum_value_def.h",
        "reflection/extension_range.h",
        "reflection/field_def.h",
        "reflection/file_def.h",
        "reflection/internal/def_pool.h",
        "reflection/internal/desc_state.h",
        "reflection/internal/enum_def.h",
        "reflection/internal/enum_reserved_range.h",
        "reflection/internal/enum_value_def.h",
        "reflection/internal/extension_range.h",
        "reflection/internal/field_def.h",
        "reflection/internal/file_def.h",
        "reflection/internal/message_def.h",
        "reflection/internal/message_reserved_range.h",
        "reflection/internal/method_def.h",
        "reflection/internal/oneof_def.h",
        "reflection/internal/service_def.h",
        "reflection/message.h",
        "reflection/message.hpp",
        "reflection/message_def.h",
        "reflection/message_reserved_range.h",
        "reflection/method_def.h",
        "reflection/oneof_def.h",
        "reflection/service_def.h",
    ],
    bootstrap_deps = [":descriptor_upb_proto"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":hash",
        ":mem",
        ":message",
        ":message_accessors",
        ":message_types",
        ":message_value",
        ":mini_descriptor",
        ":mini_descriptor_internal",
        ":mini_table",
        ":port",
    ],
)

# Aliases ######################################################################
# TODO: Remove these.

alias(
    name = "base",
    actual = "//upb/base",
    visibility = ["//visibility:public"],
)

alias(
    name = "base_internal",
    actual = "//upb/base:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "collections",
    actual = "//upb/collections",
    deprecation = "use upb:message instead",
    visibility = ["//visibility:public"],
)

alias(
    name = "hash",
    actual = "//upb/hash",
    visibility = ["//visibility:public"],
)

alias(
    name = "json",
    actual = "//upb/json",
    visibility = ["//visibility:public"],
)

alias(
    name = "lex",
    actual = "//upb/lex",
    visibility = ["//visibility:public"],
)

alias(
    name = "mem",
    actual = "//upb/mem",
    visibility = ["//visibility:public"],
)

alias(
    name = "mem_internal",
    actual = "//upb/mem:internal",
    visibility = ["//upb:__subpackages__"],
)

alias(
    name = "message",
    actual = "//upb/message",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_accessors",
    actual = "//upb/message:accessors",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_accessors_internal",
    actual = "//upb/message:accessors_internal",
    visibility = ["//upb:friends"],
)

alias(
    name = "message_copy",
    actual = "//upb/message:copy",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_internal",
    actual = "//upb/message:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_internal_types",
    actual = "//upb/message:internal_types",
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
    name = "message_tagged_ptr",
    actual = "//upb/message:tagged_ptr",
    visibility = ["//upb:friends"],
)

alias(
    name = "message_types",
    actual = "//upb/message:types",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_value",
    actual = "//upb/message:value",
    visibility = ["//visibility:public"],
)

alias(
    name = "mini_descriptor",
    actual = "//upb/mini_descriptor",
    visibility = ["//visibility:public"],
)

alias(
    name = "mini_descriptor_internal",
    actual = "//upb/mini_descriptor:internal",
    visibility = ["//upb:__subpackages__"],
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
    name = "mini_table_internal",
    actual = "//upb/mini_table:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "port",
    actual = "//upb/port",
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
    name = "wire_internal",
    actual = "//upb/wire:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "wire_reader",
    actual = "//upb/wire:reader",
    visibility = ["//visibility:public"],
)

alias(
    name = "wire_types",
    actual = "//upb/wire:types",
    visibility = ["//visibility:public"],
)

alias(
    name = "eps_copy_input_stream",
    actual = "//upb/wire:eps_copy_input_stream",
    visibility = ["//visibility:public"],
)

# Tests ########################################################################

cc_test(
    name = "def_builder_test",
    srcs = [
        "reflection/common.h",
        "reflection/def_type.h",
        "reflection/internal/def_builder.h",
        "reflection/internal/def_builder_test.cc",
    ],
    deps = [
        ":descriptor_upb_proto",
        ":hash",
        ":mem",
        ":port",
        ":reflection",
        ":reflection_internal",
        "@com_google_googletest//:gtest_main",
        "@com_google_absl//absl/strings",
    ],
)

# Internal C/C++ libraries #####################################################

cc_binary(
    name = "libupb.so",
    srcs = ["upb_so.c"],
    copts = UPB_DEFAULT_COPTS + ["-DUPB_BUILD_API"],
    linkshared = 1,
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":mem",
        ":message",
        ":message_accessors",
        ":message_split64",
        ":mini_descriptor",
        ":mini_table",
        ":port",
    ],
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
        ":base_internal",
        ":descriptor_upb_minitable_proto",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":hash",
        ":lex",
        ":mem",
        ":mem_internal",
        ":message",
        ":message_accessors",
        ":message_internal",
        ":message_internal_types",
        ":message_tagged_ptr",
        ":message_types",
        ":message_value",
        ":mini_descriptor",
        ":mini_descriptor_internal",
        ":mini_table",
        ":mini_table_compat",
        ":mini_table_internal",
        ":port",
        ":reflection",
        ":reflection_internal",
        ":wire",
        ":wire_internal",
        ":wire_reader",
        ":wire_types",
    ],
    strip_import_prefix = ["src"],
)

cc_library(
    name = "amalgamation",
    srcs = ["upb.c"],
    hdrs = ["upb.h"],
    copts = UPB_DEFAULT_COPTS,
    deps = ["@utf8_range"],
)

upb_amalgamation(
    name = "gen_php_amalgamation",
    outs = [
        "php-upb.c",
        "php-upb.h",
    ],
    libs = [
        ":base",
        ":base_internal",
        ":descriptor_upb_minitable_proto",
        ":descriptor_upb_proto_reflection",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":hash",
        ":json",
        ":lex",
        ":mem",
        ":mem_internal",
        ":message",
        ":message_accessors",
        ":message_internal",
        ":message_internal_types",
        ":message_tagged_ptr",
        ":message_types",
        ":message_value",
        ":mini_descriptor",
        ":mini_descriptor_internal",
        ":mini_table",
        ":mini_table_internal",
        ":port",
        ":reflection",
        ":reflection_internal",
        ":wire",
        ":wire_internal",
        ":wire_reader",
        ":wire_types",
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
    deps = ["@utf8_range"],
)

upb_amalgamation(
    name = "gen_ruby_amalgamation",
    outs = [
        "ruby-upb.c",
        "ruby-upb.h",
    ],
    libs = [
        ":base",
        ":base_internal",
        ":descriptor_upb_minitable_proto",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":hash",
        ":json",
        ":lex",
        ":mem",
        ":mem_internal",
        ":message",
        ":message_accessors",
        ":message_internal",
        ":message_internal_types",
        ":message_tagged_ptr",
        ":message_types",
        ":message_value",
        ":mini_descriptor",
        ":mini_descriptor_internal",
        ":mini_table",
        ":mini_table_internal",
        ":port",
        ":reflection",
        ":reflection_internal",
        ":wire",
        ":wire_internal",
        ":wire_reader",
        ":wire_types",
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
    deps = ["@utf8_range"],
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
            "**/*.c",
            "**/*.h",
            "**/*.hpp",
       ],
        exclude = [
            "**/conformance_upb.c",
            "reflection/stage0/**/*",
        ],
    ),
    visibility = [
        "//upb/cmake:__pkg__",
        "//python/dist:__pkg__",
    ]
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
#     no_string_conversion = ["_upb_MiniTable_Build"],
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
#     visibility = ["//upb:__subpackages__"],
# )
#
# end:google_only
