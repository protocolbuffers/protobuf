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
    "make_shell_script",
)
load(
    "//bazel:upb_proto_library.bzl",
    "upb_fasttable_enabled",
    "upb_proto_library",
    "upb_proto_library_copts",
    "upb_proto_reflection_library",
)

# begin:google_only
# load(
#     "//third_party/bazel_rules/rules_kotlin/kotlin/native:native_interop_hint.bzl",
#     "kt_native_interop_hint",
# )
# end:google_only

# begin:github_only
load(
    "//bazel:build_defs.bzl",
    "upb_amalgamation",
)
# end:github_only

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

upb_fasttable_enabled(
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
    packages = [],
)

# Public C/C++ libraries #######################################################

cc_library(
    name = "port",
    hdrs = [
        "upb/internal/vsnprintf_compat.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    textual_hdrs = [
        "upb/port_def.inc",
        "upb/port_undef.inc",
    ],
    visibility = ["//:__subpackages__"],
)

cc_library(
    name = "upb",
    srcs = [
        "upb/collections/map_sorter_internal.h",
        "upb/collections/message_value.h",
        "upb/internal/table.h",
        "upb/internal/unicode.h",
        "upb/msg.c",
        "upb/msg_internal.h",
        "upb/status.c",
        "upb/upb.c",
        "upb/wire/decode.c",
        "upb/wire/encode.c",
    ],
    hdrs = [
        "upb/alloc.h",
        "upb/arena.h",
        "upb/array.h",
        "upb/collections/array.h",
        "upb/decode.h",
        "upb/encode.h",
        "upb/extension_registry.h",
        "upb/mem/alloc.h",
        "upb/mem/arena.h",
        "upb/msg.h",
        "upb/status.h",
        "upb/string_view.h",
        "upb/upb.h",
        "upb/upb.hpp",
        "upb/wire/decode.h",
        "upb/wire/encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":extension_registry",
        ":fastdecode",
        ":mem",
        ":port",
        ":table_internal",
        ":unicode_internal",
        ":wire_internal",
    ],
)

cc_library(
    name = "extension_registry",
    srcs = [
        "upb/extension_registry.c",
        "upb/msg.h",
        "upb/msg_internal.h",
        "upb/string_view.h",
        "upb/upb.h",
    ],
    hdrs = [
        "upb/extension_registry.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":port",
        ":table_internal",
    ],
)

cc_library(
    name = "mini_table_internal",
    srcs = [
        "upb/mini_table/common.h",
        "upb/msg_internal.h",
    ],
    hdrs = [
        "upb/mini_table/common_internal.h",
        "upb/mini_table/encode_internal.h",
        "upb/mini_table/encode_internal.hpp",
    ],
    visibility = ["//:__subpackages__"],
    deps = [
        ":collections_internal",
        ":extension_registry",
        ":port",
        ":table_internal",
        ":upb",
    ],
)

cc_library(
    name = "mini_table",
    srcs = [
        "upb/mini_table/common.c",
        "upb/mini_table/common.h",
        "upb/mini_table/common_internal.h",
        "upb/mini_table/decode.c",
        "upb/mini_table/encode.c",
        "upb/mini_table/encode_internal.h",
        "upb/msg_internal.h",
    ],
    hdrs = [
        "upb/mini_table.h",
        "upb/mini_table/decode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":extension_registry",
        ":mini_table_internal",
        ":port",
        ":table_internal",
        ":upb",
    ],
)

cc_library(
    name = "mini_table_accessors",
    srcs = [
        "upb/mini_table/accessors.c",
        "upb/mini_table/common.h",
        "upb/msg_internal.h",
    ],
    hdrs = [
        "upb/mini_table/accessors.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        ":collections_internal",
        ":mini_table",
        ":mini_table_internal",
        ":port",
        ":table_internal",
        ":upb",
    ],
)

cc_test(
    name = "mini_table_encode_test",
    srcs = [
        "upb/internal/table.h",
        "upb/mini_table/encode_test.cc",
    ],
    deps = [
        ":extension_registry",
        ":mini_table",
        ":mini_table_internal",
        ":port",
        ":upb",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_test(
    name = "mini_table_accessors_test",
    srcs = ["upb/mini_table/accessors_test.cc"],
    deps = [
        ":collections",
        ":mini_table",
        ":mini_table_accessors",
        ":mini_table_internal",
        ":test_messages_proto2_proto_upb",
        ":test_messages_proto3_proto_upb",
        ":test_upb_proto",
        ":upb",
        "@com_google_absl//absl/container:flat_hash_set",
        "@com_google_googletest//:gtest_main",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "fastdecode",
    srcs = [
        "upb/decode.h",
        "upb/decode_fast.h",
        "upb/msg.h",
        "upb/msg_internal.h",
        "upb/wire/decode.h",
        "upb/wire/decode_fast.c",
        "upb/wire/decode_fast.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    deps = [
        ":collections_internal",
        ":extension_registry",
        ":mem_internal",
        ":port",
        ":table_internal",
        ":wire_internal",
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
    hdrs = [
        "upb/collections/array.h",
        "upb/collections/array_internal.h",
        "upb/collections/map_gencode_util.h",
        "upb/collections/message_value.h",
        "upb/extension_registry.h",
        "upb/msg.h",
        "upb/msg_internal.h",
        "upb/port_def.inc",
        "upb/port_undef.inc",
        "upb/wire/decode.h",
        "upb/wire/decode_fast.h",
        "upb/wire/encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":table_internal",
        ":upb",
    ],
)

# Common support code for C++ generated code.
cc_library(
    name = "generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    hdrs = [
        "upb/msg.h",
        "upb/msg_internal.h",
        "upb/port_def.inc",
        "upb/port_undef.inc",
        "upb/upb.hpp",
        "upb/wire/decode.h",
        "upb/wire/decode_fast.h",
        "upb/wire/encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections_internal",
        ":mini_table",
        ":table_internal",
        ":upb",
    ],
)

cc_library(
    name = "generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
    srcs = [
        "upb/reflection/common.h",
        "upb/reflection/def_pool.h",
        "upb/reflection/def_type.h",
        "upb/reflection/enum_def.h",
        "upb/reflection/enum_value_def.h",
        "upb/reflection/extension_range.h",
        "upb/reflection/field_def.h",
        "upb/reflection/file_def.h",
        "upb/reflection/message_def.h",
        "upb/reflection/method_def.h",
        "upb/reflection/oneof_def.h",
        "upb/reflection/service_def.h",
    ],
    hdrs = [
        "upb/port_def.inc",
        "upb/port_undef.inc",
        "upb/reflection/def.h",
        "upb/reflection/def_pool_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":descriptor_upb_proto",
        ":reflection",
        ":reflection_internal",
        ":table_internal",
    ],
)

upb_proto_library(
    name = "descriptor_upb_proto",
    visibility = ["//visibility:public"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

upb_proto_reflection_library(
    name = "descriptor_upb_proto_reflection",
    visibility = ["//visibility:public"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

cc_library(
    name = "collections",
    srcs = [
        "upb/collections/array.c",
        "upb/collections/array_internal.h",
        "upb/collections/map.c",
        "upb/collections/map_internal.h",
        "upb/collections/map_sorter.c",
        "upb/collections/message_value.h",
        "upb/extension_registry.h",
        "upb/msg.h",
        "upb/msg_internal.h",
    ],
    hdrs = [
        "upb/array.h",
        "upb/collections.h",
        "upb/collections/array.h",
        "upb/collections/map.h",
        "upb/collections/map_sorter_internal.h",
        "upb/map.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":port",
        ":table_internal",
    ],
)

# TODO(b/232091617): Once we can delete the deprecated forwarding headers
# (= everything in upb/) we can move this build target down into reflection/
cc_library(
    name = "reflection",
    srcs = [
        "upb/reflection/common.h",
        "upb/reflection/def_builder.c",
        "upb/reflection/def_builder_internal.h",
        "upb/reflection/def_pool.c",
        "upb/reflection/def_pool.h",
        "upb/reflection/def_pool_internal.h",
        "upb/reflection/def_type.c",
        "upb/reflection/def_type.h",
        "upb/reflection/desc_state.c",
        "upb/reflection/desc_state_internal.h",
        "upb/reflection/enum_def.c",
        "upb/reflection/enum_def.h",
        "upb/reflection/enum_def_internal.h",
        "upb/reflection/enum_value_def.c",
        "upb/reflection/enum_value_def.h",
        "upb/reflection/enum_value_def_internal.h",
        "upb/reflection/extension_range.c",
        "upb/reflection/extension_range.h",
        "upb/reflection/extension_range_internal.h",
        "upb/reflection/field_def.c",
        "upb/reflection/field_def.h",
        "upb/reflection/field_def_internal.h",
        "upb/reflection/file_def.c",
        "upb/reflection/file_def.h",
        "upb/reflection/file_def_internal.h",
        "upb/reflection/message.c",
        "upb/reflection/message_def.c",
        "upb/reflection/message_def.h",
        "upb/reflection/message_def_internal.h",
        "upb/reflection/method_def.c",
        "upb/reflection/method_def.h",
        "upb/reflection/method_def_internal.h",
        "upb/reflection/oneof_def.c",
        "upb/reflection/oneof_def.h",
        "upb/reflection/oneof_def_internal.h",
        "upb/reflection/service_def.c",
        "upb/reflection/service_def.h",
        "upb/reflection/service_def_internal.h",
    ],
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
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        ":descriptor_upb_proto",
        ":mini_table",
        ":mini_table_internal",
        ":port",
        ":table_internal",
        ":upb",
    ],
)

cc_library(
    name = "reflection_internal",
    srcs = [
        "upb/reflection/common.h",
        "upb/reflection/def_builder_internal.h",
        "upb/reflection/def_pool.h",
        "upb/reflection/def_type.h",
        "upb/reflection/desc_state_internal.h",
        "upb/reflection/enum_def.h",
        "upb/reflection/enum_value_def.h",
        "upb/reflection/extension_range.h",
        "upb/reflection/field_def.h",
        "upb/reflection/file_def.h",
        "upb/reflection/message.h",
        "upb/reflection/message_def.h",
        "upb/reflection/method_def.h",
        "upb/reflection/oneof_def.h",
        "upb/reflection/service_def.h",
    ],
    hdrs = [
        "upb/reflection/def_pool_internal.h",
        "upb/reflection/enum_def_internal.h",
        "upb/reflection/enum_value_def_internal.h",
        "upb/reflection/extension_range_internal.h",
        "upb/reflection/field_def_internal.h",
        "upb/reflection/file_def_internal.h",
        "upb/reflection/message_def_internal.h",
        "upb/reflection/method_def_internal.h",
        "upb/reflection/oneof_def_internal.h",
        "upb/reflection/service_def_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":collections",
        ":descriptor_upb_proto",
        ":mini_table",
        ":mini_table_internal",
        ":port",
        ":reflection",
        ":table_internal",
        ":upb",
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
        ":collections",
        ":port",
        ":reflection",
        ":wire_internal",
    ],
)

# TODO(b/232091617): Once we can delete the deprecated forwarding headers
# (= everything in upb/) we can move this build target down into json/
cc_library(
    name = "json",
    srcs = [
        "upb/json/decode.c",
        "upb/json/encode.c",
    ],
    hdrs = [
        "upb/json/decode.h",
        "upb/json/encode.h",
        "upb/json_decode.h",
        "upb/json_encode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
    deps = [
        ":atoi_internal",
        ":collections",
        ":port",
        ":reflection",
        ":unicode_internal",
        ":upb",
        ":wire_internal",
    ],
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
        ":port",
        ":reflection",
        ":reflection_internal",
        ":table_internal",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_generated_code",
    srcs = ["upb/test_generated_code.cc"],
    deps = [
        ":empty_upbdefs_proto",
        ":port",
        ":test_messages_proto2_proto_upb",
        ":test_messages_proto3_proto_upb",
        ":test_upb_proto",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "test_proto",
    testonly = 1,
    srcs = ["upb/test.proto"],
)

upb_proto_library(
    name = "test_upb_proto",
    testonly = 1,
    deps = [":test_proto"],
)

proto_library(
    name = "empty_proto",
    srcs = ["upb/empty.proto"],
)

upb_proto_reflection_library(
    name = "empty_upbdefs_proto",
    testonly = 1,
    deps = [":empty_proto"],
)

upb_proto_library(
    name = "test_messages_proto2_proto_upb",
    testonly = 1,
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto2_proto"],
)

upb_proto_library(
    name = "test_messages_proto3_proto_upb",
    testonly = 1,
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto3_proto"],
)

proto_library(
    name = "json_test_proto",
    testonly = 1,
    srcs = ["upb/json/test.proto"],
    deps = ["@com_google_protobuf//:struct_proto"],
)

upb_proto_library(
    name = "json_test_upb_proto",
    testonly = 1,
    deps = [":json_test_proto"],
)

upb_proto_reflection_library(
    name = "json_test_upb_proto_reflection",
    testonly = 1,
    deps = [":json_test_proto"],
)

cc_test(
    name = "json_decode_test",
    srcs = ["upb/json/decode_test.cc"],
    deps = [
        ":json",
        ":json_test_upb_proto",
        ":json_test_upb_proto_reflection",
        ":reflection",
        ":struct_upb_proto",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "json_encode_test",
    srcs = ["upb/json/encode_test.cc"],
    deps = [
        ":json",
        ":json_test_upb_proto",
        ":json_test_upb_proto_reflection",
        ":reflection",
        ":struct_upb_proto",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "msg_test",
    srcs = ["upb/msg_test.cc"],
    deps = [
        ":fuzz_test_util",
        ":json",
        ":msg_test_upb_proto",
        ":msg_test_upb_proto_reflection",
        ":reflection",
        ":test_messages_proto3_proto_upb",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

proto_library(
    name = "msg_test_proto",
    testonly = 1,
    srcs = ["upb/msg_test.proto"],
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto3_proto"],
)

upb_proto_library(
    name = "msg_test_upb_proto",
    testonly = 1,
    deps = [":msg_test_proto"],
)

proto_library(
    name = "proto3_test_proto",
    testonly = 1,
    srcs = ["upb/proto3_test.proto"],
    deps = ["@com_google_protobuf//:descriptor_proto"],
)

upb_proto_library(
    name = "proto3_test_upb_proto",
    testonly = 1,
    deps = [":proto3_test_proto"],
)

upb_proto_reflection_library(
    name = "proto3_test_upb_proto_reflection",
    testonly = 1,
    deps = [":proto3_test_proto"],
)

upb_proto_reflection_library(
    name = "msg_test_upb_proto_reflection",
    testonly = 1,
    deps = [":msg_test_proto"],
)

proto_library(
    name = "test_cpp_proto",
    srcs = ["upb/test_cpp.proto"],
    deps = ["@com_google_protobuf//:timestamp_proto"],
)

upb_proto_library(
    name = "test_cpp_upb_proto",
    deps = ["test_cpp_proto"],
)

upb_proto_reflection_library(
    name = "test_cpp_upb_proto_reflection",
    deps = ["test_cpp_proto"],
)

upb_proto_library(
    name = "struct_upb_proto",
    deps = ["@com_google_protobuf//:struct_proto"],
)

upb_proto_library(
    name = "timestamp_upb_proto",
    deps = ["@com_google_protobuf//:timestamp_proto"],
)

upb_proto_reflection_library(
    name = "timestamp_upb_proto_reflection",
    deps = ["@com_google_protobuf//:timestamp_proto"],
)

cc_test(
    name = "atoi_test",
    srcs = ["upb/internal/atoi_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":atoi_internal",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "proto3_test",
    srcs = ["upb/proto3_test.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":proto3_test_upb_proto",
        ":proto3_test_upb_proto_reflection",
        ":reflection",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_cpp",
    srcs = ["upb/test_cpp.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":json",
        ":port",
        ":reflection",
        ":test_cpp_upb_proto",
        ":test_cpp_upb_proto_reflection",
        ":timestamp_upb_proto",
        ":timestamp_upb_proto_reflection",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_table",
    srcs = ["upb/test_table.cc"],
    copts = UPB_DEFAULT_CPPOPTS,
    deps = [
        ":port",
        ":table_internal",
        ":upb",
        "@com_google_googletest//:gtest_main",
    ],
)

upb_proto_library(
    name = "conformance_proto_upb",
    testonly = 1,
    deps = ["@com_google_protobuf//conformance:conformance_proto"],
)

upb_proto_reflection_library(
    name = "conformance_proto_upbdefs",
    testonly = 1,
    deps = ["@com_google_protobuf//conformance:conformance_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto2_upbdefs",
    testonly = 1,
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto2_proto"],
)

upb_proto_reflection_library(
    name = "test_messages_proto3_upbdefs",
    testonly = 1,
    deps = ["@com_google_protobuf//src/google/protobuf:test_messages_proto3_proto"],
)

cc_binary(
    name = "conformance_upb",
    testonly = 1,
    srcs = ["upb/conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS,
    data = ["upb/conformance_upb_failures.txt"],
    deps = [
        ":conformance_proto_upb",
        ":conformance_proto_upbdefs",
        ":json",
        ":port",
        ":reflection",
        ":test_messages_proto2_upbdefs",
        ":test_messages_proto3_upbdefs",
        ":textformat",
        ":upb",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb",
    out = "test_conformance_upb.sh",
    contents = "external/com_google_protobuf/conformance/conformance_test_runner " +
               " --enforce_recommended " +
               " --failure_list ./upb/conformance_upb_failures.txt" +
               " ./conformance_upb",
)

sh_test(
    name = "test_conformance_upb",
    srcs = ["test_conformance_upb.sh"],
    data = [
        "upb/conformance_upb_failures.txt",
        ":conformance_upb",
        "@com_google_protobuf//conformance:conformance_test_runner",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)

cc_binary(
    name = "conformance_upb_dynamic_minitable",
    testonly = 1,
    srcs = ["upb/conformance_upb.c"],
    copts = UPB_DEFAULT_COPTS + [
        "-DREBUILD_MINITABLES",
    ],
    data = ["upb/conformance_upb_failures.txt"],
    deps = [
        ":conformance_proto_upb",
        ":conformance_proto_upbdefs",
        ":json",
        ":port",
        ":reflection",
        ":test_messages_proto2_upbdefs",
        ":test_messages_proto3_upbdefs",
        ":textformat",
        ":upb",
    ],
)

make_shell_script(
    name = "gen_test_conformance_upb_dynamic_minitable",
    out = "test_conformance_upb_dynamic_minitable.sh",
    contents = "external/com_google_protobuf/conformance/conformance_test_runner " +
               " --enforce_recommended " +
               " --failure_list ./upb/conformance_upb_failures.txt" +
               " ./conformance_upb_dynamic_minitable",
)

sh_test(
    name = "test_conformance_upb_dynamic_minitable",
    srcs = ["test_conformance_upb_dynamic_minitable.sh"],
    data = [
        "upb/conformance_upb_failures.txt",
        ":conformance_upb_dynamic_minitable",
        "@com_google_protobuf//conformance:conformance_test_runner",
    ],
    deps = ["@bazel_tools//tools/bash/runfiles"],
)

cc_library(
    name = "fuzz_test_util",
    testonly = 1,
    srcs = ["upb/fuzz_test_util.cc"],
    hdrs = ["upb/fuzz_test_util.h"],
    deps = [
        ":mini_table",
        ":mini_table_internal",
        ":upb",
    ],
)

# Internal C/C++ libraries #####################################################

cc_library(
    name = "collections_internal",
    srcs = [
        "upb/collections/array.h",
        "upb/collections/map_gencode_util.h",
        "upb/collections/message_value.h",
        "upb/msg.h",
        "upb/status.h",
        "upb/string_view.h",
        "upb/upb.h",
    ],
    hdrs = [
        "upb/collections/array_internal.h",
        "upb/collections/map_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [
        ":collections",
        ":mem",
        ":port",
        ":table_internal",
    ],
)

cc_library(
    name = "mem",
    srcs = [
        "upb/mem/alloc.c",
        "upb/mem/arena.c",
        "upb/mem/arena_internal.h",
    ],
    hdrs = [
        "upb/mem/alloc.h",
        "upb/mem/arena.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [":port"],
)

cc_library(
    name = "mem_internal",
    hdrs = ["upb/mem/arena_internal.h"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [
        ":mem",
        ":port",
    ],
)

cc_library(
    name = "atoi_internal",
    srcs = ["upb/internal/atoi.c"],
    hdrs = ["upb/internal/atoi.h"],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [":port"],
)

cc_library(
    name = "wire_internal",
    srcs = [
        "upb/extension_registry.h",
        "upb/msg.h",
        "upb/msg_internal.h",
        "upb/wire/decode.h",
    ],
    hdrs = [
        "upb/wire/common_internal.h",
        "upb/wire/decode_internal.h",
        "upb/wire/encode_internal.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [
        ":collections_internal",
        ":mem_internal",
        ":port",
        ":table_internal",
        "//third_party/utf8_range",
    ],
)

cc_library(
    name = "table_internal",
    srcs = [
        "upb/internal/table.c",
    ],
    hdrs = [
        "upb/internal/table.h",
        "upb/mem/alloc.h",
        "upb/mem/arena.h",
        "upb/status.h",
        "upb/string_view.h",
        "upb/upb.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [":port"],
)

cc_library(
    name = "unicode_internal",
    srcs = [
        "upb/internal/unicode.c",
    ],
    hdrs = [
        "upb/internal/unicode.h",
    ],
    copts = UPB_DEFAULT_COPTS,
    visibility = ["//:__subpackages__"],
    deps = [":port"],
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
        ":atoi_internal",
        ":collections",
        ":collections_internal",
        ":descriptor_upb_proto",
        ":extension_registry",
        ":fastdecode",
        ":mem",
        ":mem_internal",
        ":mini_table",
        ":port",
        ":reflection",
        ":reflection_internal",
        ":upb",
        ":wire_internal",
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
        ":atoi_internal",
        ":collections",
        ":collections_internal",
        ":descriptor_upb_proto",
        ":descriptor_upb_proto_reflection",
        ":extension_registry",
        ":fastdecode",
        ":json",
        ":mem",
        ":mem_internal",
        ":mini_table",
        ":port",
        ":reflection",
        ":reflection_internal",
        ":table_internal",
        ":unicode_internal",
        ":upb",
        ":wire_internal",
    ],
    prefix = "php-",
    strip_import_prefix = ["src"],
    visibility = ["@com_google_protobuf//php:__pkg__"],
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
        ":atoi_internal",
        ":collections",
        ":collections_internal",
        ":descriptor_upb_proto",
        ":extension_registry",
        ":fastdecode",
        ":json",
        ":mem",
        ":mem_internal",
        ":mini_table",
        ":port",
        ":reflection",
        ":reflection_internal",
        ":table_internal",
        ":unicode_internal",
        ":upb",
        ":wire_internal",
    ],
    prefix = "ruby-",
    strip_import_prefix = ["src"],
    visibility = ["@com_google_protobuf//ruby:__pkg__"],
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
    visibility = ["//upb/bindings/lua:__pkg__"],
)

filegroup(
    name = "cmake_files",
    srcs = glob([
        "upb/**/*",
        "third_party/**/*",
    ]),
    visibility = ["//cmake:__pkg__"],
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
#     ]) + [
#         "upb/extension_registry.h",
#     ],
#     no_string_conversion = ["upb_MiniTable_Build"],
#     strict_enums = ["upb_FieldType"],
# )
#
# kt_native_interop_hint(
#     name = "suppress_kotlin_interop",
#     compatible_with = ["//buildenv/target:non_prod"],
#     suppressed = True,
# )
#
# end:google_only
