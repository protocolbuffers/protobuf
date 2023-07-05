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
    "//bazel:build_defs.bzl",
    "UPB_DEFAULT_COPTS",
    "UPB_DEFAULT_CPPOPTS",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_proto_library",
    "upb_proto_library_copts",
    "upb_proto_reflection_library",
)
load("@bazel_skylib//rules:common_settings.bzl", "bool_flag")
load(
    "//upbc:bootstrap_compiler.bzl",
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
    "//bazel:amalgamation.bzl",
    "upb_amalgamation",
)
load("@rules_pkg//:mappings.bzl", "pkg_files")
# end:github_only

# begin:google_only
# package(default_applicable_licenses = ["//:license"])
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
    visibility = ["//cmake:__pkg__"],
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
    flag_values = {"//:fasttable_enabled": "true"},
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
        "upb/port/atomic.h",
        "upb/port/vsnprintf_compat.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "upb/port/def.inc",
        "upb/port/undef.inc",
    ],
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "upb",
    hdrs = [
        "upb/alloc.h",
        "upb/arena.h",
        "upb/array.h",
        "upb/base/descriptor_constants.h",
        "upb/base/status.h",
        "upb/base/string_view.h",
        "upb/collections/array.h",
        "upb/decode.h",
        "upb/encode.h",
        "upb/extension_registry.h",
        "upb/map.h",
        "upb/mem/alloc.h",
        "upb/mem/arena.h",
        "upb/message/extension_internal.h",
        "upb/message/message.h",
        "upb/msg.h",
        "upb/status.h",
        "upb/string_view.h",
        "upb/upb.h",
        "upb/upb.hpp",
        "upb/wire/common.h",
        "upb/wire/decode.h",
        "upb/wire/encode.h",
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
        "upb/base/status.c",
    ],
    hdrs = [
        "upb/base/descriptor_constants.h",
        "upb/base/log2.h",
        "upb/base/status.h",
        "upb/base/string_view.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [":port"],
)

cc_library(
    name = "message",
    hdrs = [
        "upb/message/message.h",
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
    visibility = ["//:friends"],
)

alias(
    name = "mini_table_internal",
    actual = "//upb/mini_table:internal",
    visibility = ["//:friends"],
)

cc_library(
    name = "message_internal",
    srcs = [
        "upb/message/message.c",
    ],
    hdrs = [
        "upb/message/extension_internal.h",
        "upb/message/internal.h",
        "upb/message/message.h",
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
    hdrs = ["upb/message/tagged_ptr.h"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:friends"],
    deps = [
        ":port",
    ],
)

cc_library(
    name = "message_accessors_internal",
    hdrs = [
        "upb/message/accessors_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:friends"],
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
        "upb/message/accessors.c",
        "upb/message/accessors_internal.h",
    ],
    hdrs = [
        "upb/message/accessors.h",
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
        "upb/message/promote.c",
    ],
    hdrs = [
        "upb/message/promote.h",
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
        "upb/message/copy.c",
    ],
    hdrs = [
        "upb/message/copy.h",
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
        "upb/message/accessors_split64.h",
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
    srcs = ["upb/message/accessors_test.cc"],
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
    srcs = ["upb/message/promote_test.cc"],
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
    srcs = ["upb/message/copy_test.cc"],
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
    hdrs = ["upb/generated_code_support.h"],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "upb/port/def.inc",
        "upb/port/undef.inc",
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
        "upb/message/message.h",
        "upb/port/def.inc",
        "upb/port/undef.inc",
        "upb/upb.hpp",
        "upb/wire/decode.h",
        "upb/wire/decode_fast.h",
        "upb/wire/encode.h",
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
        "upb/port/def.inc",
        "upb/port/undef.inc",
        "upb/reflection/def.h",
        "upb/reflection/def_pool_internal.h",
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
    base_dir = "upb/reflection/",
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
        "upb/collections/array.h",
        "upb/collections/map.h",
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
        "upb/message/internal/map_entry.h",
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
        "upb/collections/array.c",
        "upb/collections/map.c",
        "upb/collections/map_sorter.c",
    ],
    hdrs = [
        "upb/collections/array.h",
        "upb/collections/array_internal.h",
        "upb/collections/map.h",
        "upb/collections/map_gencode_util.h",
        "upb/collections/map_internal.h",
        "upb/collections/map_sorter_internal.h",
        "upb/collections/message_value.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
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
        "upb/collections/array_split64.h",
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
        "upb/def.h",
        "upb/def.hpp",
        "upb/reflection.h",
        "upb/reflection.hpp",
        "upb/reflection/def.h",
        "upb/reflection/def.hpp",
        "upb/reflection/message.h",
        "upb/reflection/message.hpp",
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
        "upb/reflection/def_builder.c",
        "upb/reflection/def_pool.c",
        "upb/reflection/def_type.c",
        "upb/reflection/desc_state.c",
        "upb/reflection/enum_def.c",
        "upb/reflection/enum_reserved_range.c",
        "upb/reflection/enum_value_def.c",
        "upb/reflection/extension_range.c",
        "upb/reflection/field_def.c",
        "upb/reflection/file_def.c",
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
        "upb/reflection/def_builder_internal.h",
        "upb/reflection/def_pool.h",
        "upb/reflection/def_pool_internal.h",
        "upb/reflection/def_type.h",
        "upb/reflection/desc_state_internal.h",
        "upb/reflection/enum_def.h",
        "upb/reflection/enum_def_internal.h",
        "upb/reflection/enum_reserved_range.h",
        "upb/reflection/enum_reserved_range_internal.h",
        "upb/reflection/enum_value_def.h",
        "upb/reflection/enum_value_def_internal.h",
        "upb/reflection/extension_range.h",
        "upb/reflection/extension_range_internal.h",
        "upb/reflection/field_def.h",
        "upb/reflection/field_def_internal.h",
        "upb/reflection/file_def.h",
        "upb/reflection/file_def_internal.h",
        "upb/reflection/message.h",
        "upb/reflection/message.hpp",
        "upb/reflection/message_def.h",
        "upb/reflection/message_def_internal.h",
        "upb/reflection/message_reserved_range.h",
        "upb/reflection/message_reserved_range_internal.h",
        "upb/reflection/method_def.h",
        "upb/reflection/method_def_internal.h",
        "upb/reflection/oneof_def.h",
        "upb/reflection/oneof_def_internal.h",
        "upb/reflection/service_def.h",
        "upb/reflection/service_def_internal.h",
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
        "upb/text/encode.c",
    ],
    hdrs = [
        "upb/text/encode.h",
        "upb/text_encode.h",
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
        "upb/reflection/common.h",
        "upb/reflection/def_builder_internal.h",
        "upb/reflection/def_builder_test.cc",
        "upb/reflection/def_type.h",
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
    srcs = ["upb/collections/test.cc"],
    deps = [
        ":collections",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "message_test",
    srcs = ["upb/message/test.cc"],
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
    srcs = ["upb/message/test.proto"],
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
    srcs = ["upb/lex/atoi_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":lex",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "hash_test",
    srcs = ["upb/hash/test.cc"],
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
        "upb/mem/alloc.h",
        "upb/mem/arena.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [
        ":mem_internal",
        ":port",
    ],
)

cc_test(
    name = "arena_test",
    srcs = ["upb/mem/arena_test.cc"],
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
        "upb/mem/alloc.c",
        "upb/mem/arena.c",
    ],
    hdrs = [
        "upb/mem/alloc.h",
        "upb/mem/arena.h",
        "upb/mem/arena_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [":port"],
)

cc_library(
    name = "wire",
    hdrs = [
        "upb/wire/decode.h",
        "upb/wire/encode.h",
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
        "upb/wire/decode.c",
        "upb/wire/decode_fast.c",
        "upb/wire/encode.c",
    ],
    hdrs = [
        "upb/wire/common.h",
        "upb/wire/common_internal.h",
        "upb/wire/decode.h",
        "upb/wire/decode_fast.h",
        "upb/wire/decode_internal.h",
        "upb/wire/encode.h",
        "upb/wire/swap_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
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
    hdrs = ["upb/wire/types.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "eps_copy_input_stream",
    srcs = ["upb/wire/eps_copy_input_stream.c"],
    hdrs = ["upb/wire/eps_copy_input_stream.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":mem",
        ":port",
    ],
)

cc_library(
    name = "wire_reader",
    srcs = [
        "upb/wire/reader.c",
        "upb/wire/swap_internal.h",
    ],
    hdrs = ["upb/wire/reader.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":eps_copy_input_stream",
        ":port",
        ":wire_types",
    ],
)

cc_test(
    name = "eps_copy_input_stream_test",
    srcs = ["upb/wire/eps_copy_input_stream_test.cc"],
    deps = [
        ":eps_copy_input_stream",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "hash",
    srcs = [
        "upb/hash/common.c",
    ],
    hdrs = [
        "upb/hash/common.h",
        "upb/hash/int_table.h",
        "upb/hash/str_table.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [
        ":base",
        ":mem",
        ":port",
    ],
)

cc_library(
    name = "lex",
    srcs = [
        "upb/lex/atoi.c",
        "upb/lex/round_trip.c",
        "upb/lex/strtod.c",
        "upb/lex/unicode.c",
    ],
    hdrs = [
        "upb/lex/atoi.h",
        "upb/lex/round_trip.h",
        "upb/lex/strtod.h",
        "upb/lex/unicode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [":port"],
)

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
    visibility = ["//lua:__pkg__"],
)

filegroup(
    name = "cmake_files",
    srcs = glob([
        "upb/**/*",
        "third_party/**/*",
    ]),
    visibility = ["//cmake:__pkg__"],
)


pkg_files(
   name = "upb_source_files",
   srcs = glob(
       [
           "upb/**/*.c",
           "upb/**/*.h",
           "upb/**/*.hpp",
           "upb/**/*.inc",
      ],
       exclude = [
           "upb/**/conformance_upb.c",
           "upb/reflection/stage0/**/*",
       ],
   ),
   strip_prefix = "",
   visibility = ["//python/dist:__pkg__"],
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
#     visibility = ["//:__subpackages__"],
# )
#
# end:google_only
