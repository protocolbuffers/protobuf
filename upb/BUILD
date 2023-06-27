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

load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)
load(
    "//upb/bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_library_copts",
    "upb_proto_reflection_library",
)
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load(
    "//upb/upbc:bootstrap_compiler.bzl",
    "bootstrap_cc_library",
    "bootstrap_upb_proto_library",
)

# begin:google_only
# load(
#     "//third_party/bazel_rules/rules_kotlin/kotlin/native:native_interop_hint.bzl",
#     "kt_native_interop_hint",
# )
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

# Public C/C++ libraries #######################################################

cc_library(
    name = "port",
    hdrs = [
        "port/atomic.h",
        "port/vsnprintf_compat.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "port/def.inc",
        "port/undef.inc",
    ],
    visibility = ["//upb:__subpackages__"],
)

cc_library(
    name = "upb",
    hdrs = [
        "alloc.h",
        "arena.h",
        "array.h",
        "base/descriptor_constants.h",
        "base/status.h",
        "base/string_view.h",
        "collections/array.h",
        "decode.h",
        "encode.h",
        "extension_registry.h",
        "map.h",
        "mem/alloc.h",
        "mem/arena.h",
        "message/extension_internal.h",
        "message/message.h",
        "msg.h",
        "status.h",
        "string_view.h",
        "upb.h",
        "upb.hpp",
        "wire/common.h",
        "wire/decode.h",
        "wire/encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":collections_internal",
        ":fastdecode",
        ":hash",
        ":lex",
        ":mem",
        ":message_internal",
        ":port",
        ":wire",
        "//upb/mini_table",
    ],
)

cc_library(
    name = "base",
    srcs = [
        "base/status.c",
    ],
    hdrs = [
        "base/descriptor_constants.h",
        "base/log2.h",
        "base/status.h",
        "base/string_view.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__subpackages__"],
    deps = [":port"],
)

cc_library(
    name = "message",
    hdrs = [
        "message/message.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":mem",
        ":message_internal",
        ":port",
        "//upb/mini_table",
    ],
)

alias(
    name = "mini_table",
    actual = "//upb/mini_table",
    visibility = ["//upb:friends"],
)

alias(
    name = "mini_table_internal",
    actual = "//upb/mini_table:internal",
    visibility = ["//upb:friends"],
)

cc_library(
    name = "message_internal",
    srcs = [
        "message/message.c",
    ],
    hdrs = [
        "message/extension_internal.h",
        "message/internal.h",
        "message/message.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":hash",
        ":mem",
        ":port",
        "//upb/mini_table",
    ],
)

cc_library(
    name = "message_tagged_ptr",
    hdrs = ["message/tagged_ptr.h"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:friends"],
    deps = [
        ":port",
    ],
)

cc_library(
    name = "message_accessors_internal",
    hdrs = [
        "message/accessors_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:friends"],
    deps = [
        ":collections_internal",
        ":message_internal",
        ":mini_table_internal",
        ":port",
    ],
)

cc_library(
    name = "message_accessors",
    srcs = [
        "message/accessors.c",
        "message/accessors_internal.h",
    ],
    hdrs = [
        "message/accessors.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":eps_copy_input_stream",
        ":message_internal",
        ":mini_table",
        ":port",
        ":upb",
        ":wire",
        ":wire_reader",
        "//upb/mini_table:internal",
    ],
)

cc_library(
    name = "message_promote",
    srcs = [
        "message/promote.c",
    ],
    hdrs = [
        "message/promote.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":eps_copy_input_stream",
        ":hash",
        ":message_accessors",
        ":message_internal",
        ":port",
        ":upb",
        ":wire",
        ":wire_reader",
        "//upb/mini_table",
    ],
)

cc_library(
    name = "message_copy",
    srcs = [
        "message/copy.c",
    ],
    hdrs = [
        "message/copy.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":message_accessors",
        ":message_internal",
        ":port",
        ":upb",
        "//upb/mini_table",
    ],
)

cc_library(
    name = "message_split64",
    hdrs = [
        "message/accessors_split64.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":message_accessors",
        ":port",
    ],
)

cc_test(
    name = "message_accessors_test",
    srcs = ["message/accessors_test.cc"],
    deps = [
        ":collections",
        ":message_accessors",
        ":port",
        ":upb",
        "//upb/mini_descriptor",
        "//upb/mini_descriptor:encode_internal",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "message_promote_test",
    srcs = ["message/promote_test.cc"],
    deps = [
        ":collections",
        ":message_accessors",
        ":message_copy",
        ":message_promote",
        ":port",
        ":upb",
        "//upb/mini_descriptor:encode_internal",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "message_copy_test",
    srcs = ["message/copy_test.cc"],
    deps = [
        ":collections",
        ":message_accessors",
        ":message_copy",
        ":upb",
        "//upb/mini_table",
        "//upb/test:test_messages_proto2_upb_proto",
        "//upb/test:test_messages_proto3_upb_proto",
        "//upb/test:test_upb_proto",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "fastdecode",
    copts = UPB_DEFAULT_COPTS,
    deps = [
        ":base",
        ":collections_internal",
        ":hash",
        ":mem_internal",
        ":message_internal",
        ":port",
        ":wire",
        "//upb/mini_table",
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
        "port/def.inc",
        "port/undef.inc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":message_accessors",
        ":message_accessors_internal",
        ":message_internal",
        ":upb",
        ":wire_internal",
        "//upb/mini_descriptor",
        "//upb/mini_table",
    ],
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = [
        "message/message.h",
        "port/def.inc",
        "port/undef.inc",
        "upb.hpp",
        "wire/decode.h",
        "wire/decode_fast.h",
        "wire/encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":collections_internal",
        ":hash",
        ":message_copy",
        ":upb",
        "//upb/mini_table",
    ],
)

cc_library(
    name = "generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = [
        "port/def.inc",
        "port/undef.inc",
        "reflection/def.h",
        "reflection/def_pool_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":reflection_internal",
        "//upb/mini_descriptor",
    ],
)

bootstrap_upb_proto_library(
    name = "descriptor_upb_proto",
    base_dir = "reflection/",
    # TODO(b/289127200): Export 'net/proto2/proto/descriptor.upb.h' and remove "-layering_check".
    features = ["-layering_check"],
    google3_src_files = ["net/proto2/proto/descriptor.proto"],
    google3_src_rules = ["//net/proto2/proto:descriptor_proto_source"],
    oss_src_files = ["google/protobuf/descriptor.proto"],
    oss_src_rules = ["@com_google_protobuf//:descriptor_proto_srcs"],
    oss_strip_prefix = "third_party/protobuf/github/bootstrap/src",
    proto_lib_deps = ["@com_google_protobuf//:descriptor_proto"],
    visibility = ["//visibility:public"],
)

upb_proto_reflection_library(
    name = "descriptor_upb_proto_reflection",
    visibility = ["//visibility:public"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

cc_library(
    name = "collections",
    hdrs = [
        "collections/array.h",
        "collections/map.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":base",
        ":collections_internal",
        ":mem",
        ":port",
    ],
)

cc_library(
    name = "message_rep_internal",
    hdrs = [
        "message/internal/map_entry.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    deps = [
        ":base",
        ":hash",
    ],
)

cc_library(
    name = "collections_internal",
    srcs = [
        "collections/array.c",
        "collections/map.c",
        "collections/map_sorter.c",
    ],
    hdrs = [
        "collections/array.h",
        "collections/array_internal.h",
        "collections/map.h",
        "collections/map_gencode_util.h",
        "collections/map_internal.h",
        "collections/map_sorter_internal.h",
        "collections/message_value.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__subpackages__"],
    deps = [
        ":base",
        ":hash",
        ":mem",
        ":message_internal",
        ":message_rep_internal",
        ":message_tagged_ptr",
        ":port",
        "//upb/mini_table",
    ],
)

cc_library(
    name = "collections_split64",
    hdrs = [
        "collections/array_split64.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        ":port",
    ],
)

# TODO(b/232091617): Once we can delete the deprecated forwarding headers
# (= everything in upb/) we can move this build target down into reflection/
bootstrap_cc_library(
    name = "reflection",
    hdrs = [
        "def.h",
        "def.hpp",
        "reflection.h",
        "reflection.hpp",
        "reflection/def.h",
        "reflection/def.hpp",
        "reflection/message.h",
        "reflection/message.hpp",
    ],
    bootstrap_deps = [":reflection_internal"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        ":port",
        ":upb",
    ],
)

bootstrap_cc_library(
    name = "reflection_internal",
    srcs = [
        "reflection/def_builder.c",
        "reflection/def_pool.c",
        "reflection/def_type.c",
        "reflection/desc_state.c",
        "reflection/enum_def.c",
        "reflection/enum_reserved_range.c",
        "reflection/enum_value_def.c",
        "reflection/extension_range.c",
        "reflection/field_def.c",
        "reflection/file_def.c",
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
        "reflection/def_builder_internal.h",
        "reflection/def_pool.h",
        "reflection/def_pool_internal.h",
        "reflection/def_type.h",
        "reflection/desc_state_internal.h",
        "reflection/enum_def.h",
        "reflection/enum_def_internal.h",
        "reflection/enum_reserved_range.h",
        "reflection/enum_reserved_range_internal.h",
        "reflection/enum_value_def.h",
        "reflection/enum_value_def_internal.h",
        "reflection/extension_range.h",
        "reflection/extension_range_internal.h",
        "reflection/field_def.h",
        "reflection/field_def_internal.h",
        "reflection/file_def.h",
        "reflection/file_def_internal.h",
        "reflection/message.h",
        "reflection/message.hpp",
        "reflection/message_def.h",
        "reflection/message_def_internal.h",
        "reflection/message_reserved_range.h",
        "reflection/message_reserved_range_internal.h",
        "reflection/method_def.h",
        "reflection/method_def_internal.h",
        "reflection/oneof_def.h",
        "reflection/oneof_def_internal.h",
        "reflection/service_def.h",
        "reflection/service_def_internal.h",
    ],
    bootstrap_deps = [":descriptor_upb_proto"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        ":hash",
        ":message_accessors",
        ":port",
        ":upb",
        "//upb/mini_descriptor",
        "//upb/mini_descriptor:encode_internal",
        "//upb/mini_descriptor:internal",
        "//upb/mini_table",
    ],
)

cc_library(
    name = "textformat",
    srcs = [
        "text/encode.c",
    ],
    hdrs = [
        "text/encode.h",
        "text_encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":eps_copy_input_stream",
        ":lex",
        ":port",
        ":reflection",
        ":wire",
        ":wire_reader",
        ":wire_types",
    ],
)

alias(
    name = "json",
    actual = "//upb/json",
    visibility = ["//visibility:public"],
)

# Tests ########################################################################

cc_test(
    name = "def_builder_test",
    srcs = [
        "reflection/common.h",
        "reflection/def_builder_internal.h",
        "reflection/def_builder_test.cc",
        "reflection/def_type.h",
    ],
    deps = [
        ":descriptor_upb_proto",
        ":hash",
        ":port",
        ":reflection",
        ":reflection_internal",
        ":upb",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "collections_test",
    srcs = ["collections/test.cc"],
    deps = [
        ":collections",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "message_test",
    srcs = ["message/test.cc"],
    deps = [
        ":json",
        ":message_test_upb_proto",
        ":message_test_upb_proto_reflection",
        ":reflection",
        ":upb",
        "//upb/json",
        "//upb/test:fuzz_util",
        "//upb/test:test_messages_proto3_upb_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "message_test_proto",
    testonly = 1,
    srcs = ["message/test.proto"],
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto3_proto"],
)

upb_proto_library(
    name = "message_test_upb_proto",
    testonly = 1,
    deps = [":message_test_proto"],
)

upb_proto_reflection_library(
    name = "message_test_upb_proto_reflection",
    testonly = 1,
    deps = [":message_test_proto"],
)

cc_test(
    name = "atoi_test",
    srcs = ["lex/atoi_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":lex",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "hash_test",
    srcs = ["hash/test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":hash",
        ":port",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

# Internal C/C++ libraries #####################################################

cc_library(
    name = "mem",
    hdrs = [
        "mem/alloc.h",
        "mem/arena.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__subpackages__"],
    deps = [
        ":mem_internal",
        ":port",
    ],
)

cc_test(
    name = "arena_test",
    srcs = ["mem/arena_test.cc"],
    deps = [
        ":port",
        ":upb",
        "@com_google_absl//absl/random",
        "@com_google_absl//absl/random:distributions",
        "@com_google_absl//absl/synchronization",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "mem_internal",
    srcs = [
        "mem/alloc.c",
        "mem/arena.c",
    ],
    hdrs = [
        "mem/alloc.h",
        "mem/arena.h",
        "mem/arena_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__subpackages__"],
    deps = [":port"],
)

cc_library(
    name = "wire",
    hdrs = [
        "wire/decode.h",
        "wire/encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":mem",
        ":message_internal",
        ":port",
        ":wire_internal",
        "//upb/mini_table",
    ],
)

cc_library(
    name = "wire_internal",
    srcs = [
        "wire/decode.c",
        "wire/decode_fast.c",
        "wire/encode.c",
    ],
    hdrs = [
        "wire/common.h",
        "wire/common_internal.h",
        "wire/decode.h",
        "wire/decode_fast.h",
        "wire/decode_internal.h",
        "wire/encode.h",
        "wire/swap_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__subpackages__"],
    deps = [
        ":base",
        ":collections_internal",
        ":eps_copy_input_stream",
        ":mem_internal",
        ":message_accessors_internal",
        ":message_internal",
        ":message_rep_internal",
        ":port",
        ":wire_reader",
        ":wire_types",
        "//upb/mini_table",
        "@utf8_range",
    ],
)

cc_library(
    name = "wire_types",
    hdrs = ["wire/types.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "eps_copy_input_stream",
    srcs = ["wire/eps_copy_input_stream.c"],
    hdrs = ["wire/eps_copy_input_stream.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":mem",
        ":port",
    ],
)

cc_library(
    name = "wire_reader",
    srcs = [
        "wire/reader.c",
        "wire/swap_internal.h",
    ],
    hdrs = ["wire/reader.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":port",
        ":wire_types",
    ],
)

cc_test(
    name = "eps_copy_input_stream_test",
    srcs = ["wire/eps_copy_input_stream_test.cc"],
    deps = [
        ":eps_copy_input_stream",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "hash",
    srcs = [
        "hash/common.c",
    ],
    hdrs = [
        "hash/common.h",
        "hash/int_table.h",
        "hash/str_table.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__subpackages__"],
    deps = [
        ":base",
        ":mem",
        ":port",
    ],
)

cc_library(
    name = "lex",
    srcs = [
        "lex/atoi.c",
        "lex/round_trip.c",
        "lex/strtod.c",
        "lex/unicode.c",
    ],
    hdrs = [
        "lex/atoi.h",
        "lex/round_trip.h",
        "lex/strtod.h",
        "lex/unicode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//upb:__subpackages__"],
    deps = [":port"],
)

cc_binary(
    name = "libupb.so",
    srcs = ["upb_so.c"],
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
        ":mini_table",
        ":port",
        "//upb/mini_descriptor",
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
        ":collections_internal",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":fastdecode",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":hash",
        ":lex",
        ":mem_internal",
        ":message_accessors",
        ":message_internal",
        ":message_rep_internal",
        ":message_tagged_ptr",
        ":port",
        ":reflection_internal",
        ":reflection",
        ":upb",
        ":wire_internal",
        ":wire_reader",
        ":wire_types",
        "//upb/mini_descriptor:encode_internal",
        "//upb/mini_descriptor:internal",
        "//upb/mini_descriptor:mini_descriptor",
        "//upb/mini_table:internal",
        "//upb/mini_table:mini_table",
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
        ":collections_internal",
        ":descriptor_upb_proto_reflection",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":fastdecode",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":hash",
        ":json",
        ":lex",
        ":mem_internal",
        ":message_accessors",
        ":message_internal",
        ":message_rep_internal",
        ":message_tagged_ptr",
        ":port",
        ":reflection_internal",
        ":reflection",
        ":upb",
        ":wire_internal",
        ":wire_reader",
        ":wire_types",
        "//upb/mini_descriptor:encode_internal",
        "//upb/mini_descriptor:internal",
        "//upb/mini_descriptor:mini_descriptor",
        "//upb/mini_table:internal",
        "//upb/mini_table:mini_table",
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
        ":collections_internal",
        ":descriptor_upb_proto",
        ":eps_copy_input_stream",
        ":fastdecode",
        ":generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ":hash",
        ":json",
        ":lex",
        ":mem_internal",
        ":message_accessors",
        ":message_internal",
        ":message_rep_internal",
        ":message_tagged_ptr",
        ":port",
        ":reflection_internal",
        ":reflection",
        ":upb",
        ":wire_internal",
        ":wire_reader",
        ":wire_types",
        "//upb/mini_descriptor:encode_internal",
        "//upb/mini_descriptor:internal",
        "//upb/mini_descriptor:mini_descriptor",
        "//upb/mini_table:internal",
        "//upb/mini_table:mini_table",
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
            "**/*.c",
            "**/*.h",
            "**/*.hpp",
            "**/*.inc",
       ],
        exclude = [
            "upb/**/conformance_upb.c",
            "upb/reflection/stage0/**/*",
        ],
    ),
    visibility = [
        "//cmake:__pkg__",
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
#     strict_enums = ["upb_FieldType"],
#     visibility = ["//upb:__subpackages__"],
# )
#
# end:google_only
