# Bazel (http://bazel.io/) BUILD file for Protobuf.

licenses(["notice"])

################################################################################
# Protobuf Runtime Library
################################################################################

COPTS = [
    "-DHAVE_PTHREAD",
    "-Wall",
    "-Wwrite-strings",
    "-Woverloaded-virtual",
    "-Wno-sign-compare",
    "-Wno-error=unused-function",
]

# Bazel should provide portable link_opts for pthread.
LINK_OPTS = ["-lpthread"]

cc_library(
    name = "protobuf_lite",
    srcs = [
        # AUTOGEN(protobuf_lite_srcs)
        "src/google/protobuf/arena.cc",
        "src/google/protobuf/arenastring.cc",
        "src/google/protobuf/extension_set.cc",
        "src/google/protobuf/generated_message_util.cc",
        "src/google/protobuf/io/coded_stream.cc",
        "src/google/protobuf/io/zero_copy_stream.cc",
        "src/google/protobuf/io/zero_copy_stream_impl_lite.cc",
        "src/google/protobuf/message_lite.cc",
        "src/google/protobuf/repeated_field.cc",
        "src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc",
        "src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc",
        "src/google/protobuf/stubs/bytestream.cc",
        "src/google/protobuf/stubs/common.cc",
        "src/google/protobuf/stubs/int128.cc",
        "src/google/protobuf/stubs/once.cc",
        "src/google/protobuf/stubs/status.cc",
        "src/google/protobuf/stubs/statusor.cc",
        "src/google/protobuf/stubs/stringpiece.cc",
        "src/google/protobuf/stubs/stringprintf.cc",
        "src/google/protobuf/stubs/structurally_valid.cc",
        "src/google/protobuf/stubs/strutil.cc",
        "src/google/protobuf/stubs/time.cc",
        "src/google/protobuf/wire_format_lite.cc",
    ],
    copts = COPTS,
    includes = ["src/"],
    linkopts = LINK_OPTS,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "protobuf",
    srcs = [
        # AUTOGEN(protobuf_srcs)
        "src/google/protobuf/any.cc",
        "src/google/protobuf/any.pb.cc",
        "src/google/protobuf/api.pb.cc",
        "src/google/protobuf/compiler/importer.cc",
        "src/google/protobuf/compiler/parser.cc",
        "src/google/protobuf/descriptor.cc",
        "src/google/protobuf/descriptor.pb.cc",
        "src/google/protobuf/descriptor_database.cc",
        "src/google/protobuf/duration.pb.cc",
        "src/google/protobuf/dynamic_message.cc",
        "src/google/protobuf/empty.pb.cc",
        "src/google/protobuf/extension_set_heavy.cc",
        "src/google/protobuf/field_mask.pb.cc",
        "src/google/protobuf/generated_message_reflection.cc",
        "src/google/protobuf/io/gzip_stream.cc",
        "src/google/protobuf/io/printer.cc",
        "src/google/protobuf/io/strtod.cc",
        "src/google/protobuf/io/tokenizer.cc",
        "src/google/protobuf/io/zero_copy_stream_impl.cc",
        "src/google/protobuf/map_field.cc",
        "src/google/protobuf/message.cc",
        "src/google/protobuf/reflection_ops.cc",
        "src/google/protobuf/service.cc",
        "src/google/protobuf/source_context.pb.cc",
        "src/google/protobuf/struct.pb.cc",
        "src/google/protobuf/stubs/mathlimits.cc",
        "src/google/protobuf/stubs/substitute.cc",
        "src/google/protobuf/text_format.cc",
        "src/google/protobuf/timestamp.pb.cc",
        "src/google/protobuf/type.pb.cc",
        "src/google/protobuf/unknown_field_set.cc",
        "src/google/protobuf/util/field_comparator.cc",
        "src/google/protobuf/util/field_mask_util.cc",
        "src/google/protobuf/util/internal/datapiece.cc",
        "src/google/protobuf/util/internal/default_value_objectwriter.cc",
        "src/google/protobuf/util/internal/error_listener.cc",
        "src/google/protobuf/util/internal/field_mask_utility.cc",
        "src/google/protobuf/util/internal/json_escaping.cc",
        "src/google/protobuf/util/internal/json_objectwriter.cc",
        "src/google/protobuf/util/internal/json_stream_parser.cc",
        "src/google/protobuf/util/internal/object_writer.cc",
        "src/google/protobuf/util/internal/protostream_objectsource.cc",
        "src/google/protobuf/util/internal/protostream_objectwriter.cc",
        "src/google/protobuf/util/internal/type_info.cc",
        "src/google/protobuf/util/internal/type_info_test_helper.cc",
        "src/google/protobuf/util/internal/utility.cc",
        "src/google/protobuf/util/json_util.cc",
        "src/google/protobuf/util/message_differencer.cc",
        "src/google/protobuf/util/time_util.cc",
        "src/google/protobuf/util/type_resolver_util.cc",
        "src/google/protobuf/wire_format.cc",
        "src/google/protobuf/wrappers.pb.cc",
    ],
    copts = COPTS,
    includes = ["src/"],
    linkopts = LINK_OPTS,
    visibility = ["//visibility:public"],
    deps = [":protobuf_lite"],
)

objc_library(
    name = "protobuf_objc",
    hdrs = ["objectivec/GPBProtocolBuffers.h"],
    includes = ["objectivec"],
    non_arc_srcs = ["objectivec/GPBProtocolBuffers.m"],
    visibility = ["//visibility:public"],
)

WELL_KNOWN_PROTOS = [
    # AUTOGEN(well_known_protos)
    "google/protobuf/any.proto",
    "google/protobuf/api.proto",
    "google/protobuf/compiler/plugin.proto",
    "google/protobuf/descriptor.proto",
    "google/protobuf/duration.proto",
    "google/protobuf/empty.proto",
    "google/protobuf/field_mask.proto",
    "google/protobuf/source_context.proto",
    "google/protobuf/struct.proto",
    "google/protobuf/timestamp.proto",
    "google/protobuf/type.proto",
    "google/protobuf/wrappers.proto",
]

################################################################################
# Protocol Buffers Compiler
################################################################################

cc_library(
    name = "protoc_lib",
    srcs = [
        # AUTOGEN(protoc_lib_srcs)
        "src/google/protobuf/compiler/code_generator.cc",
        "src/google/protobuf/compiler/command_line_interface.cc",
        "src/google/protobuf/compiler/cpp/cpp_enum.cc",
        "src/google/protobuf/compiler/cpp/cpp_enum_field.cc",
        "src/google/protobuf/compiler/cpp/cpp_extension.cc",
        "src/google/protobuf/compiler/cpp/cpp_field.cc",
        "src/google/protobuf/compiler/cpp/cpp_file.cc",
        "src/google/protobuf/compiler/cpp/cpp_generator.cc",
        "src/google/protobuf/compiler/cpp/cpp_helpers.cc",
        "src/google/protobuf/compiler/cpp/cpp_map_field.cc",
        "src/google/protobuf/compiler/cpp/cpp_message.cc",
        "src/google/protobuf/compiler/cpp/cpp_message_field.cc",
        "src/google/protobuf/compiler/cpp/cpp_primitive_field.cc",
        "src/google/protobuf/compiler/cpp/cpp_service.cc",
        "src/google/protobuf/compiler/cpp/cpp_string_field.cc",
        "src/google/protobuf/compiler/csharp/csharp_enum.cc",
        "src/google/protobuf/compiler/csharp/csharp_enum_field.cc",
        "src/google/protobuf/compiler/csharp/csharp_field_base.cc",
        "src/google/protobuf/compiler/csharp/csharp_generator.cc",
        "src/google/protobuf/compiler/csharp/csharp_helpers.cc",
        "src/google/protobuf/compiler/csharp/csharp_map_field.cc",
        "src/google/protobuf/compiler/csharp/csharp_message.cc",
        "src/google/protobuf/compiler/csharp/csharp_message_field.cc",
        "src/google/protobuf/compiler/csharp/csharp_primitive_field.cc",
        "src/google/protobuf/compiler/csharp/csharp_repeated_enum_field.cc",
        "src/google/protobuf/compiler/csharp/csharp_repeated_message_field.cc",
        "src/google/protobuf/compiler/csharp/csharp_repeated_primitive_field.cc",
        "src/google/protobuf/compiler/csharp/csharp_source_generator_base.cc",
        "src/google/protobuf/compiler/csharp/csharp_umbrella_class.cc",
        "src/google/protobuf/compiler/csharp/csharp_wrapper_field.cc",
        "src/google/protobuf/compiler/java/java_context.cc",
        "src/google/protobuf/compiler/java/java_doc_comment.cc",
        "src/google/protobuf/compiler/java/java_enum.cc",
        "src/google/protobuf/compiler/java/java_enum_field.cc",
        "src/google/protobuf/compiler/java/java_enum_field_lite.cc",
        "src/google/protobuf/compiler/java/java_enum_lite.cc",
        "src/google/protobuf/compiler/java/java_extension.cc",
        "src/google/protobuf/compiler/java/java_field.cc",
        "src/google/protobuf/compiler/java/java_file.cc",
        "src/google/protobuf/compiler/java/java_generator.cc",
        "src/google/protobuf/compiler/java/java_generator_factory.cc",
        "src/google/protobuf/compiler/java/java_helpers.cc",
        "src/google/protobuf/compiler/java/java_lazy_message_field.cc",
        "src/google/protobuf/compiler/java/java_lazy_message_field_lite.cc",
        "src/google/protobuf/compiler/java/java_map_field.cc",
        "src/google/protobuf/compiler/java/java_map_field_lite.cc",
        "src/google/protobuf/compiler/java/java_message.cc",
        "src/google/protobuf/compiler/java/java_message_builder.cc",
        "src/google/protobuf/compiler/java/java_message_builder_lite.cc",
        "src/google/protobuf/compiler/java/java_message_field.cc",
        "src/google/protobuf/compiler/java/java_message_field_lite.cc",
        "src/google/protobuf/compiler/java/java_message_lite.cc",
        "src/google/protobuf/compiler/java/java_name_resolver.cc",
        "src/google/protobuf/compiler/java/java_primitive_field.cc",
        "src/google/protobuf/compiler/java/java_primitive_field_lite.cc",
        "src/google/protobuf/compiler/java/java_service.cc",
        "src/google/protobuf/compiler/java/java_shared_code_generator.cc",
        "src/google/protobuf/compiler/java/java_string_field.cc",
        "src/google/protobuf/compiler/java/java_string_field_lite.cc",
        "src/google/protobuf/compiler/javanano/javanano_enum.cc",
        "src/google/protobuf/compiler/javanano/javanano_enum_field.cc",
        "src/google/protobuf/compiler/javanano/javanano_extension.cc",
        "src/google/protobuf/compiler/javanano/javanano_field.cc",
        "src/google/protobuf/compiler/javanano/javanano_file.cc",
        "src/google/protobuf/compiler/javanano/javanano_generator.cc",
        "src/google/protobuf/compiler/javanano/javanano_helpers.cc",
        "src/google/protobuf/compiler/javanano/javanano_map_field.cc",
        "src/google/protobuf/compiler/javanano/javanano_message.cc",
        "src/google/protobuf/compiler/javanano/javanano_message_field.cc",
        "src/google/protobuf/compiler/javanano/javanano_primitive_field.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_enum.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_enum_field.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_extension.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_field.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_file.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_generator.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_helpers.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_map_field.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_message.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_message_field.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_oneof.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_primitive_field.cc",
        "src/google/protobuf/compiler/plugin.cc",
        "src/google/protobuf/compiler/plugin.pb.cc",
        "src/google/protobuf/compiler/python/python_generator.cc",
        "src/google/protobuf/compiler/ruby/ruby_generator.cc",
        "src/google/protobuf/compiler/subprocess.cc",
        "src/google/protobuf/compiler/zip_writer.cc",
    ],
    copts = COPTS,
    includes = ["src/"],
    linkopts = LINK_OPTS,
    visibility = ["//visibility:public"],
    deps = [":protobuf"],
)

cc_binary(
    name = "protoc",
    srcs = ["src/google/protobuf/compiler/main.cc"],
    linkopts = LINK_OPTS,
    visibility = ["//visibility:public"],
    deps = [":protoc_lib"],
)

################################################################################
# Tests
################################################################################

LITE_TEST_PROTOS = [
    # AUTOGEN(lite_test_protos)
    "google/protobuf/map_lite_unittest.proto",
    "google/protobuf/unittest_import_lite.proto",
    "google/protobuf/unittest_import_public_lite.proto",
    "google/protobuf/unittest_lite.proto",
    "google/protobuf/unittest_no_arena_lite.proto",
]

TEST_PROTOS = [
    # AUTOGEN(test_protos)
    "google/protobuf/any_test.proto",
    "google/protobuf/compiler/cpp/cpp_test_bad_identifiers.proto",
    "google/protobuf/compiler/cpp/cpp_test_large_enum_value.proto",
    "google/protobuf/map_proto2_unittest.proto",
    "google/protobuf/map_unittest.proto",
    "google/protobuf/unittest.proto",
    "google/protobuf/unittest_arena.proto",
    "google/protobuf/unittest_custom_options.proto",
    "google/protobuf/unittest_drop_unknown_fields.proto",
    "google/protobuf/unittest_embed_optimize_for.proto",
    "google/protobuf/unittest_empty.proto",
    "google/protobuf/unittest_enormous_descriptor.proto",
    "google/protobuf/unittest_import.proto",
    "google/protobuf/unittest_import_public.proto",
    "google/protobuf/unittest_lite_imports_nonlite.proto",
    "google/protobuf/unittest_mset.proto",
    "google/protobuf/unittest_mset_wire_format.proto",
    "google/protobuf/unittest_no_arena.proto",
    "google/protobuf/unittest_no_arena_import.proto",
    "google/protobuf/unittest_no_field_presence.proto",
    "google/protobuf/unittest_no_generic_services.proto",
    "google/protobuf/unittest_optimize_for.proto",
    "google/protobuf/unittest_preserve_unknown_enum.proto",
    "google/protobuf/unittest_preserve_unknown_enum2.proto",
    "google/protobuf/unittest_proto3_arena.proto",
    "google/protobuf/unittest_well_known_types.proto",
    "google/protobuf/util/internal/testdata/anys.proto",
    "google/protobuf/util/internal/testdata/books.proto",
    "google/protobuf/util/internal/testdata/default_value.proto",
    "google/protobuf/util/internal/testdata/default_value_test.proto",
    "google/protobuf/util/internal/testdata/field_mask.proto",
    "google/protobuf/util/internal/testdata/maps.proto",
    "google/protobuf/util/internal/testdata/oneofs.proto",
    "google/protobuf/util/internal/testdata/struct.proto",
    "google/protobuf/util/internal/testdata/timestamp_duration.proto",
    "google/protobuf/util/json_format_proto3.proto",
]

PROTOS = LITE_TEST_PROTOS + TEST_PROTOS

INPUTS = PROTOS + WELL_KNOWN_PROTOS

OUTPUTS = ["src/" + x[:-5] + "pb.h" for x in PROTOS] + \
          ["src/" + x[:-5] + "pb.cc" for x in PROTOS]

genrule(
    name = "gen_test_protos",
    srcs = ["src/" + x for x in INPUTS],
    outs = OUTPUTS,
    cmd =
        "$(location :protoc) --cpp_out=$(@D)/src" +
        "".join([" -I" + x + "=$(location src/" + x + ")" for x in INPUTS]) +
        "".join([" $(location src/" + x + ")" for x in PROTOS]),
    tools = [":protoc"],
)

COMMON_TEST_SRCS = [
    # AUTOGEN(common_test_srcs)
    "src/google/protobuf/arena_test_util.cc",
    "src/google/protobuf/map_test_util.cc",
    "src/google/protobuf/test_util.cc",
    "src/google/protobuf/testing/file.cc",
    "src/google/protobuf/testing/googletest.cc",
]

cc_binary(
    name = "test_plugin",
    srcs = [
        # AUTOGEN(test_plugin_srcs)
        "src/google/protobuf/compiler/mock_code_generator.cc",
        "src/google/protobuf/compiler/test_plugin.cc",
        "src/google/protobuf/testing/file.cc",
    ],
    deps = [
        ":protobuf",
        ":protoc_lib",
        "//external:gtest",
    ],
)

cc_test(
    name = "protobuf_test",
    srcs = OUTPUTS + COMMON_TEST_SRCS + [
        # AUTOGEN(test_srcs)
        "src/google/protobuf/any_test.cc",
        "src/google/protobuf/arena_unittest.cc",
        "src/google/protobuf/arenastring_unittest.cc",
        "src/google/protobuf/compiler/command_line_interface_unittest.cc",
        "src/google/protobuf/compiler/cpp/cpp_bootstrap_unittest.cc",
        "src/google/protobuf/compiler/cpp/cpp_plugin_unittest.cc",
        "src/google/protobuf/compiler/cpp/cpp_unittest.cc",
        "src/google/protobuf/compiler/csharp/csharp_generator_unittest.cc",
        "src/google/protobuf/compiler/importer_unittest.cc",
        "src/google/protobuf/compiler/java/java_doc_comment_unittest.cc",
        "src/google/protobuf/compiler/java/java_plugin_unittest.cc",
        "src/google/protobuf/compiler/mock_code_generator.cc",
        "src/google/protobuf/compiler/objectivec/objectivec_helpers_unittest.cc",
        "src/google/protobuf/compiler/parser_unittest.cc",
        "src/google/protobuf/compiler/python/python_plugin_unittest.cc",
        "src/google/protobuf/compiler/ruby/ruby_generator_unittest.cc",
        "src/google/protobuf/descriptor_database_unittest.cc",
        "src/google/protobuf/descriptor_unittest.cc",
        "src/google/protobuf/drop_unknown_fields_test.cc",
        "src/google/protobuf/dynamic_message_unittest.cc",
        "src/google/protobuf/extension_set_unittest.cc",
        "src/google/protobuf/generated_message_reflection_unittest.cc",
        "src/google/protobuf/io/coded_stream_unittest.cc",
        "src/google/protobuf/io/printer_unittest.cc",
        "src/google/protobuf/io/tokenizer_unittest.cc",
        "src/google/protobuf/io/zero_copy_stream_unittest.cc",
        "src/google/protobuf/map_field_test.cc",
        "src/google/protobuf/map_test.cc",
        "src/google/protobuf/message_unittest.cc",
        "src/google/protobuf/no_field_presence_test.cc",
        "src/google/protobuf/preserve_unknown_enum_test.cc",
        "src/google/protobuf/proto3_arena_unittest.cc",
        "src/google/protobuf/reflection_ops_unittest.cc",
        "src/google/protobuf/repeated_field_reflection_unittest.cc",
        "src/google/protobuf/repeated_field_unittest.cc",
        "src/google/protobuf/stubs/bytestream_unittest.cc",
        "src/google/protobuf/stubs/common_unittest.cc",
        "src/google/protobuf/stubs/int128_unittest.cc",
        "src/google/protobuf/stubs/once_unittest.cc",
        "src/google/protobuf/stubs/status_test.cc",
        "src/google/protobuf/stubs/statusor_test.cc",
        "src/google/protobuf/stubs/stringpiece_unittest.cc",
        "src/google/protobuf/stubs/stringprintf_unittest.cc",
        "src/google/protobuf/stubs/structurally_valid_unittest.cc",
        "src/google/protobuf/stubs/strutil_unittest.cc",
        "src/google/protobuf/stubs/template_util_unittest.cc",
        "src/google/protobuf/stubs/time_test.cc",
        "src/google/protobuf/stubs/type_traits_unittest.cc",
        "src/google/protobuf/text_format_unittest.cc",
        "src/google/protobuf/unknown_field_set_unittest.cc",
        "src/google/protobuf/util/field_comparator_test.cc",
        "src/google/protobuf/util/field_mask_util_test.cc",
        "src/google/protobuf/util/internal/default_value_objectwriter_test.cc",
        "src/google/protobuf/util/internal/json_objectwriter_test.cc",
        "src/google/protobuf/util/internal/json_stream_parser_test.cc",
        "src/google/protobuf/util/internal/protostream_objectsource_test.cc",
        "src/google/protobuf/util/internal/protostream_objectwriter_test.cc",
        "src/google/protobuf/util/internal/type_info_test_helper.cc",
        "src/google/protobuf/util/json_util_test.cc",
        "src/google/protobuf/util/time_util_test.cc",
        "src/google/protobuf/util/type_resolver_util_test.cc",
        "src/google/protobuf/well_known_types_unittest.cc",
        "src/google/protobuf/wire_format_unittest.cc",
    ],
    copts = COPTS,
    data = [
        ":test_plugin",
    ],
    includes = [
        "src/",
    ],
    linkopts = LINK_OPTS,
    deps = [
        ":protobuf",
        ":protoc_lib",
        "//external:gtest_main",
    ],
)

