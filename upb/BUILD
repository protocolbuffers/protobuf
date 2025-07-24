# Copyright (c) 2009-2021, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_python//python:defs.bzl", "py_binary")
load("//bazel/private:upb_proto_library_internal/copts.bzl", "upb_proto_library_copts")
load("//upb/bazel:amalgamation.bzl", "upb_amalgamation")
load("//upb/bazel:build_defs.bzl", "UPB_DEFAULT_COPTS")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

exports_files(["LICENSE"])

exports_files(
    [
        "BUILD",
    ],
    visibility = ["//upb/cmake:__pkg__"],
)

config_setting(
    name = "windows",
    constraint_values = ["@platforms//os:windows"],
    visibility = [
        "//lua:__pkg__",
        "//python:__pkg__",
        "//upb:__subpackages__",
        "//upb_generator:__subpackages__",
    ],
)

bool_flag(
    name = "fasttable_enabled",
    build_setting_default = False,
    visibility = ["//bazel:__pkg__"],
)

config_setting(
    name = "fasttable_enabled_setting",
    flag_values = {"//upb:fasttable_enabled": "true"},
    visibility = [
        "//lua:__pkg__",
        "//python:__pkg__",
        "//upb:__subpackages__",
        "//upb_generator:__subpackages__",
    ],
)

upb_proto_library_copts(
    name = "upb_proto_library_copts",
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//bazel:__pkg__"],
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
    name = "generated_code_support",
    hdrs = ["generated_code_support.h"],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "//upb/port:inc",
    ],
    visibility = [
        "//pkg:__pkg__",
        "//upb/reflection:__pkg__",
        "//upb_generator:__subpackages__",
    ],
    deps = [
        "//upb/base",
        "//upb/mem",
        "//upb/message",
        "//upb/message:internal",
        "//upb/mini_descriptor",
        "//upb/mini_table",
        "//upb/wire",
    ] + select({
        ":fasttable_enabled_setting": [
            "//upb/wire/decode_fast:field_parsers",
        ],
        "//conditions:default": [],
    }),
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_cpp_support",
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "//upb/port:inc",
    ],
    visibility = ["//hpb/bazel:__pkg__"],
)

# Amalgamation #################################################################

upb_amalgamation(
    name = "gen_amalgamation",
    outs = [
        "upb.c",
        "upb.h",
    ],
    libs = [
        ":generated_code_support",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf:descriptor_upb_minitable_proto",
        "//upb/base",
        "//upb/base:internal",
        "//upb/hash:hash",
        "//upb/lex:lex",
        "//upb/mem",
        "//upb/mem:internal",
        "//upb/message",
        "//upb/message:compare",
        "//upb/message:copy",
        "//upb/message:internal",
        "//upb/message:iterator",
        "//upb/message:types",
        "//upb/mini_descriptor",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/reflection",
        "//upb/reflection:descriptor_upb_proto",
        "//upb/reflection:internal",
        "//upb/text:debug",
        "//upb/text:internal",
        "//upb/wire",
        "//upb/wire:decoder",
        "//upb/wire:eps_copy_input_stream",
        "//upb/wire:reader",
        "//upb/wire:reader_internal",
        "//upb/wire:writer",
    ],
    strip_import_prefix = ["src"],
    visibility = ["//rust:__pkg__"],
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
        ":generated_code_support",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf:descriptor_upb_minitable_proto",
        "//src/google/protobuf:descriptor_upb_reflection_proto",
        "//upb/base",
        "//upb/base:internal",
        "//upb/hash:hash",
        "//upb/json",
        "//upb/lex:lex",
        "//upb/mem",
        "//upb/mem:internal",
        "//upb/message",
        "//upb/message:compare",
        "//upb/message:copy",
        "//upb/message:internal",
        "//upb/message:iterator",
        "//upb/message:types",
        "//upb/mini_descriptor",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/reflection",
        "//upb/reflection:descriptor_upb_proto",
        "//upb/reflection:internal",
        "//upb/wire",
        "//upb/wire:decoder",
        "//upb/wire:eps_copy_input_stream",
        "//upb/wire:reader",
        "//upb/wire:reader_internal",
        "//upb/wire:writer",
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
        ":generated_code_support",
        "//src/google/protobuf:descriptor_upb_c_proto",
        "//src/google/protobuf:descriptor_upb_minitable_proto",
        "//upb/base",
        "//upb/base:internal",
        "//upb/hash:hash",
        "//upb/json",
        "//upb/lex:lex",
        "//upb/mem",
        "//upb/mem:internal",
        "//upb/message",
        "//upb/message:compare",
        "//upb/message:copy",
        "//upb/message:internal",
        "//upb/message:iterator",
        "//upb/message:types",
        "//upb/mini_descriptor",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table",
        "//upb/mini_table:internal",
        "//upb/port",
        "//upb/reflection",
        "//upb/reflection:descriptor_upb_proto",
        "//upb/reflection:internal",
        "//upb/util:def_to_proto",
        "//upb/wire",
        "//upb/wire:decoder",
        "//upb/wire:eps_copy_input_stream",
        "//upb/wire:reader",
        "//upb/wire:reader_internal",
        "//upb/wire:writer",
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

cc_library(
    name = "test_util",
    testonly = True,
    visibility = ["//pkg:__pkg__"],
    deps = [
        "//upb/test:fuzz_util",
        "//upb/test:parse_text_proto",
        "//upb/util:def_to_proto_test_lib",
        "//upb/wire/test_util:field_types",
        "//upb/wire/test_util:make_mini_table",
        "//upb/wire/test_util:wire_message",
    ],
)
