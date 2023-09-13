# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_python//python:defs.bzl", "py_binary")
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")
load("//upb/bazel:upb_proto_library.bzl", "upb_proto_reflection_library")
load("//upb/bazel:upb_proto_library_internal/copts.bzl", "upb_proto_library_copts")
load(
    "//upb/upbc:bootstrap_compiler.bzl",
    "bootstrap_cc_library",
    "bootstrap_upb_proto_library",
)

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

# This is a stub library to keep gRPC happy. Do not use it for any reason,
# use the smaller targets below instead.
cc_library(
    name = "upb",
    hdrs = [
        "upb/upb.hpp",
    ],
    copts = UPB_DEFAULT_COPTS,
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
    hdrs = ["upb/generated_code_support.h"],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "//upb/upb/port:inc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":collections",
        ":collections_internal",
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
        "//upb/upb/port:inc",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = [
        "upb/reflection/def.h",
        "upb/reflection/internal/def_pool.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "//upb/upb/port:inc",
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
    base_dir = "upb/reflection/",
    # TODO(b/289127200): Export 'net/proto2/proto/descriptor.upb.h' and remove "-layering_check".
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

# TODO(b/232091617): Once we can delete the deprecated forwarding headers
# (= everything in upb/) we can move this build target down into reflection/
bootstrap_cc_library(
    name = "reflection",
    hdrs = [
        "upb/reflection/def.h",
        "upb/reflection/def.hpp",
        "upb/reflection/message.h",
        "upb/reflection/message.hpp",
    ],
    bootstrap_deps = [":reflection_internal"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":collections",
        ":mem",
        ":port",
    ],
)

bootstrap_cc_library(
    name = "reflection_internal",
    srcs = [
        "upb/reflection/def_pool.c",
        "upb/reflection/def_type.c",
        "upb/reflection/desc_state.c",
        "upb/reflection/enum_def.c",
        "upb/reflection/enum_reserved_range.c",
        "upb/reflection/enum_value_def.c",
        "upb/reflection/extension_range.c",
        "upb/reflection/field_def.c",
        "upb/reflection/file_def.c",
        "upb/reflection/internal/def_builder.c",
        "upb/reflection/internal/def_builder.h",
        "upb/reflection/internal/strdup2.c",
        "upb/reflection/internal/strdup2.h",
        "upb/reflection/message.c",
        "upb/reflection/message_def.c",
        "upb/reflection/message_reserved_range.c",
        "upb/reflection/method_def.c",
        "upb/reflection/oneof_def.c",
        "upb/reflection/service_def.c",
    ],
    hdrs = [
        "upb/reflection/common.h",
        "upb/reflection/def.h",
        "upb/reflection/def.hpp",
        "upb/reflection/def_pool.h",
        "upb/reflection/def_type.h",
        "upb/reflection/enum_def.h",
        "upb/reflection/enum_reserved_range.h",
        "upb/reflection/enum_value_def.h",
        "upb/reflection/extension_range.h",
        "upb/reflection/field_def.h",
        "upb/reflection/file_def.h",
        "upb/reflection/internal/def_pool.h",
        "upb/reflection/internal/desc_state.h",
        "upb/reflection/internal/enum_def.h",
        "upb/reflection/internal/enum_reserved_range.h",
        "upb/reflection/internal/enum_value_def.h",
        "upb/reflection/internal/extension_range.h",
        "upb/reflection/internal/field_def.h",
        "upb/reflection/internal/file_def.h",
        "upb/reflection/internal/message_def.h",
        "upb/reflection/internal/message_reserved_range.h",
        "upb/reflection/internal/method_def.h",
        "upb/reflection/internal/oneof_def.h",
        "upb/reflection/internal/service_def.h",
        "upb/reflection/message.h",
        "upb/reflection/message.hpp",
        "upb/reflection/message_def.h",
        "upb/reflection/message_reserved_range.h",
        "upb/reflection/method_def.h",
        "upb/reflection/oneof_def.h",
        "upb/reflection/service_def.h",
    ],
    bootstrap_deps = [":descriptor_upb_proto"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":collections",
        ":hash",
        ":mem",
        ":message",
        ":message_accessors",
        ":mini_descriptor",
        ":mini_descriptor_internal",
        ":mini_table",
        ":port",
    ],
)

# Aliases ######################################################################
# TODO(b/295870230): Remove these.

alias(
    name = "base",
    actual = "//upb/upb/base",
    visibility = ["//visibility:public"],
)

alias(
    name = "base_internal",
    actual = "//upb/upb/base:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "collections",
    actual = "//upb/upb/collections",
    visibility = ["//visibility:public"],
)

alias(
    name = "collections_internal",
    actual = "//upb/upb/collections:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "collections_split64",
    actual = "//upb/upb/collections:split64",
    visibility = ["//visibility:public"],
)

alias(
    name = "hash",
    actual = "//upb/upb/hash",
    visibility = ["//visibility:public"],
)

alias(
    name = "json",
    actual = "//upb/upb/json",
    visibility = ["//visibility:public"],
)

alias(
    name = "lex",
    actual = "//upb/upb/lex",
    visibility = ["//visibility:public"],
)

alias(
    name = "mem",
    actual = "//upb/upb/mem",
    visibility = ["//visibility:public"],
)

alias(
    name = "mem_internal",
    actual = "//upb/upb/mem:internal",
    visibility = ["//upb:__subpackages__"],
)

alias(
    name = "message",
    actual = "//upb/upb/message",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_accessors",
    actual = "//upb/upb/message:accessors",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_accessors_internal",
    actual = "//upb/upb/message:accessors_internal",
    visibility = ["//upb:friends"],
)

alias(
    name = "message_copy",
    actual = "//upb/upb/message:copy",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_internal",
    actual = "//upb/upb/message:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_internal_types",
    actual = "//upb/upb/message:internal_types",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_promote",
    actual = "//upb/upb/message:promote",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_split64",
    actual = "//upb/upb/message:split64",
    visibility = ["//visibility:public"],
)

alias(
    name = "message_tagged_ptr",
    actual = "//upb/upb/message:tagged_ptr",
    visibility = ["//upb:friends"],
)

alias(
    name = "message_types",
    actual = "//upb/upb/message:types",
    visibility = ["//visibility:public"],
)

alias(
    name = "mini_descriptor",
    actual = "//upb/upb/mini_descriptor",
    visibility = ["//visibility:public"],
)

alias(
    name = "mini_descriptor_internal",
    actual = "//upb/upb/mini_descriptor:internal",
    visibility = ["//upb:__subpackages__"],
)

alias(
    name = "mini_table",
    actual = "//upb/upb/mini_table",
    visibility = ["//upb:friends"],
)

# begin:google_only
# alias(
#     name = "mini_table_compat",
#     actual = "//upb/upb/mini_table:compat",
#     compatible_with = ["//buildenv/target:non_prod"],
#     visibility = ["//upb:friends"],
# )
# end:google_only

alias(
    name = "mini_table_internal",
    actual = "//upb/upb/mini_table:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "port",
    actual = "//upb/upb/port",
    visibility = ["//visibility:public"],
)

alias(
    name = "text",
    actual = "//upb/upb/text",
    visibility = ["//visibility:public"],
)

alias(
    name = "wire",
    actual = "//upb/upb/wire",
    visibility = ["//visibility:public"],
)

alias(
    name = "wire_internal",
    actual = "//upb/upb/wire:internal",
    visibility = ["//visibility:public"],
)

alias(
    name = "wire_reader",
    actual = "//upb/upb/wire:reader",
    visibility = ["//visibility:public"],
)

alias(
    name = "wire_types",
    actual = "//upb/upb/wire:types",
    visibility = ["//visibility:public"],
)

alias(
    name = "eps_copy_input_stream",
    actual = "//upb/upb/wire:eps_copy_input_stream",
    visibility = ["//visibility:public"],
)

# Tests ########################################################################

cc_test(
    name = "def_builder_test",
    srcs = [
        "upb/reflection/common.h",
        "upb/reflection/def_type.h",
        "upb/reflection/internal/def_builder.h",
        "upb/reflection/internal/def_builder_test.cc",
    ],
    deps = [
        ":descriptor_upb_proto",
        ":hash",
        ":mem",
        ":port",
        ":reflection",
        ":reflection_internal",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

# Internal C/C++ libraries #####################################################

cc_binary(
    name = "libupb.so",
    srcs = ["upb/upb_so.c"],
    copts = UPB_DEFAULT_COPTS + ["-DUPB_BUILD_API"],
    linkshared = 1,
    linkstatic = 1,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        ":collections_split64",
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
        ":collections_internal",
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
        ":collections_internal",
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
        ":collections_internal",
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
    visibility = ["//upb/lua:__pkg__"],
)

filegroup(
    name = "source_files",
    srcs = glob(
        [
            "upb/**/*.c",
            "upb/**/*.h",
            "upb/**/*.hpp",
       ],
        exclude = [
            "upb/**/conformance_upb.c",
            "upb/reflection/stage0/**/*",
        ],
    ),
    visibility = [
        "//upb/cmake:__pkg__",
        "//upb/python/dist:__pkg__",
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
