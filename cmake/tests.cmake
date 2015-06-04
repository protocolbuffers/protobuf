include_directories(
  ${protobuf_source_dir}/gtest/include
  ${protobuf_source_dir}/gtest)

add_library(gtest STATIC ${protobuf_source_dir}/gtest/src/gtest-all.cc)
add_library(gtest_main STATIC ${protobuf_source_dir}/gtest/src/gtest_main.cc)
target_link_libraries(gtest_main gtest)

set(lite_test_protos
  google/protobuf/map_lite_unittest.proto
  google/protobuf/unittest_import_lite.proto
  google/protobuf/unittest_import_public_lite.proto
  google/protobuf/unittest_lite.proto
)

set(tests_protos
  google/protobuf/any_test.proto
  google/protobuf/compiler/cpp/cpp_test_bad_identifiers.proto
  google/protobuf/compiler/cpp/cpp_test_large_enum_value.proto
  google/protobuf/map_proto2_unittest.proto
  google/protobuf/map_unittest.proto
  google/protobuf/unittest.proto
  google/protobuf/unittest_arena.proto
  google/protobuf/unittest_custom_options.proto
  google/protobuf/unittest_drop_unknown_fields.proto
  google/protobuf/unittest_embed_optimize_for.proto
  google/protobuf/unittest_empty.proto
  google/protobuf/unittest_import.proto
  google/protobuf/unittest_import_public.proto
  google/protobuf/unittest_lite_imports_nonlite.proto
  google/protobuf/unittest_mset.proto
  google/protobuf/unittest_no_arena.proto
  google/protobuf/unittest_no_arena_import.proto
  google/protobuf/unittest_no_field_presence.proto
  google/protobuf/unittest_no_generic_services.proto
  google/protobuf/unittest_optimize_for.proto
  google/protobuf/unittest_preserve_unknown_enum.proto
  google/protobuf/unittest_preserve_unknown_enum2.proto
  google/protobuf/unittest_proto3_arena.proto
  google/protobuf/unittest_well_known_types.proto
)

macro(compile_proto_file filename)
  get_filename_component(dirname ${filename} PATH)
  get_filename_component(basename ${filename} NAME_WE)
  add_custom_command(
    OUTPUT ${protobuf_source_dir}/src/${dirname}/${basename}.pb.cc
    COMMAND protoc ${protobuf_source_dir}/src/${dirname}/${basename}.proto
        --proto_path=${protobuf_source_dir}/src
        --cpp_out=${protobuf_source_dir}/src
    DEPENDS protoc
  )
endmacro(compile_proto_file)

set(lite_test_proto_files)
foreach(proto_file ${lite_test_protos})
  compile_proto_file(${proto_file})
  string(REPLACE .proto .pb.cc pb_file ${proto_file})
  set(lite_test_proto_files ${lite_test_proto_files}
      ${protobuf_source_dir}/src/${pb_file})
endforeach(proto_file)

set(tests_proto_files)
foreach(proto_file ${tests_protos})
  compile_proto_file(${proto_file})
  string(REPLACE .proto .pb.cc pb_file ${proto_file})
  set(tests_proto_files ${tests_proto_files}
      ${protobuf_source_dir}/src/${pb_file})
endforeach(proto_file)

set(common_test_files
  ${protobuf_source_dir}/src/google/protobuf/arena_test_util.cc
  ${protobuf_source_dir}/src/google/protobuf/map_test_util.cc
  ${protobuf_source_dir}/src/google/protobuf/test_util.cc
  ${protobuf_source_dir}/src/google/protobuf/testing/file.cc
  ${protobuf_source_dir}/src/google/protobuf/testing/googletest.cc
)

set(tests_files
  ${protobuf_source_dir}/src/google/protobuf/any_test.cc
  ${protobuf_source_dir}/src/google/protobuf/arena_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/arenastring_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/command_line_interface_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_bootstrap_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_plugin_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/cpp/cpp_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/csharp/csharp_generator_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/importer_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_doc_comment_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/java/java_plugin_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/mock_code_generator.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/objectivec/objectivec_helpers_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/parser_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/python/python_plugin_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/compiler/ruby/ruby_generator_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/descriptor_database_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/descriptor_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/drop_unknown_fields_test.cc
  ${protobuf_source_dir}/src/google/protobuf/dynamic_message_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/extension_set_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/generated_message_reflection_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/io/coded_stream_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/io/printer_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/io/tokenizer_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/map_field_test.cc
  ${protobuf_source_dir}/src/google/protobuf/map_test.cc
  ${protobuf_source_dir}/src/google/protobuf/message_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/no_field_presence_test.cc
  ${protobuf_source_dir}/src/google/protobuf/preserve_unknown_enum_test.cc
  ${protobuf_source_dir}/src/google/protobuf/proto3_arena_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/reflection_ops_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/repeated_field_reflection_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/repeated_field_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/common_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/once_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/stringprintf_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/structurally_valid_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/strutil_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/template_util_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/type_traits_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/text_format_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/unknown_field_set_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/well_known_types_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/wire_format_unittest.cc
)

add_executable(tests ${tests_files} ${common_test_files} ${tests_proto_files} ${lite_test_proto_files})
target_link_libraries(tests libprotoc libprotobuf gtest_main)

set(test_plugin_files
  ${protobuf_source_dir}/src/google/protobuf/compiler/mock_code_generator.cc
  ${protobuf_source_dir}/src/google/protobuf/testing/file.cc
  ${protobuf_source_dir}/src/google/protobuf/testing/file.h
  ${protobuf_source_dir}/src/google/protobuf/compiler/test_plugin.cc
)

add_executable(test_plugin ${test_plugin_files})
target_link_libraries(test_plugin libprotoc libprotobuf gtest)

set(lite_test_files
  ${protobuf_source_dir}/src/google/protobuf/arena_test_util.cc
  ${protobuf_source_dir}/src/google/protobuf/lite_unittest.cc
  ${protobuf_source_dir}/src/google/protobuf/map_lite_test_util.cc
  ${protobuf_source_dir}/src/google/protobuf/test_util_lite.cc
)
add_executable(lite-test ${lite_test_files} ${lite_test_proto_files})
target_link_libraries(lite-test libprotobuf-lite)
