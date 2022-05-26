option(protobuf_USE_EXTERNAL_GTEST "Use external Google Test (i.e. not the one in third_party/googletest)" OFF)

option(protobuf_TEST_XML_OUTDIR "Output directory for XML logs from tests." "")

option(protobuf_ABSOLUTE_TEST_PLUGIN_PATH
  "Using absolute test_plugin path in tests" ON)
mark_as_advanced(protobuf_ABSOLUTE_TEST_PLUGIN_PATH)

if (protobuf_USE_EXTERNAL_GTEST)
  find_package(GTest REQUIRED)
else()
  if (NOT EXISTS "${protobuf_SOURCE_DIR}/third_party/googletest/CMakeLists.txt")
    message(FATAL_ERROR
            "Cannot find third_party/googletest directory that's needed to "
            "build tests. If you use git, make sure you have cloned submodules:\n"
            "  git submodule update --init --recursive\n"
            "If instead you want to skip tests, run cmake with:\n"
            "  cmake -Dprotobuf_BUILD_TESTS=OFF\n")
  endif()

  set(googlemock_source_dir "${protobuf_SOURCE_DIR}/third_party/googletest/googlemock")
  set(googletest_source_dir "${protobuf_SOURCE_DIR}/third_party/googletest/googletest")
  include_directories(
    ${googlemock_source_dir}
    ${googletest_source_dir}
    ${googletest_source_dir}/include
    ${googlemock_source_dir}/include
  )

  add_library(gmock STATIC
    "${googlemock_source_dir}/src/gmock-all.cc"
    "${googletest_source_dir}/src/gtest-all.cc"
  )
  target_link_libraries(gmock ${CMAKE_THREAD_LIBS_INIT})
  add_library(gmock_main STATIC "${googlemock_source_dir}/src/gmock_main.cc")
  target_link_libraries(gmock_main gmock)

  add_library(GTest::gmock ALIAS gmock)
  add_library(GTest::gmock_main ALIAS gmock_main)
endif()

set(lite_test_protos
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_lite_unittest.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_import_lite.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_import_public_lite.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_lite.proto
)

set(tests_protos
  ${protobuf_SOURCE_DIR}/src/google/protobuf/any_test.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/test_bad_identifiers.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/test_large_enum_value.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_proto2_unittest.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_unittest.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_arena.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_custom_options.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_drop_unknown_fields.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_embed_optimize_for.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_empty.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_enormous_descriptor.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_import.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_import_public.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_lazy_dependencies.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_lazy_dependencies_custom_option.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_lazy_dependencies_enum.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_lite_imports_nonlite.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_mset.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_mset_wire_format.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_no_field_presence.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_no_generic_services.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_optimize_for.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_preserve_unknown_enum.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_preserve_unknown_enum2.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_proto3.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_proto3_arena.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_proto3_arena_lite.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_proto3_lite.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_proto3_optional.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unittest_well_known_types.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/anys.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/books.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/default_value.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/default_value_test.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/field_mask.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/maps.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/oneofs.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/proto3.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/struct.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/timestamp_duration.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/testdata/wrappers.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/json_format.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/json_format_proto3.proto
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/message_differencer_unittest.proto
)

macro(compile_proto_file filename)
  string(REPLACE .proto .pb.cc pb_file ${filename})
  add_custom_command(
    OUTPUT ${pb_file}
    DEPENDS ${protobuf_PROTOC_EXE} ${filename}
    COMMAND ${protobuf_PROTOC_EXE} ${filename}
        --proto_path=${protobuf_SOURCE_DIR}/src
        --cpp_out=${protobuf_SOURCE_DIR}/src
        --experimental_allow_proto3_optional
  )
endmacro(compile_proto_file)

set(lite_test_proto_files)
foreach(proto_file ${lite_test_protos})
  compile_proto_file(${proto_file})
  set(lite_test_proto_files ${lite_test_proto_files} ${pb_file})
endforeach(proto_file)

set(tests_proto_files)
foreach(proto_file ${tests_protos})
  compile_proto_file(${proto_file})
  set(tests_proto_files ${tests_proto_files} ${pb_file})
endforeach(proto_file)

set(common_lite_test_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arena_test_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_lite_test_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_util_lite.cc
)

add_library(protobuf-lite-test-common STATIC
  ${common_lite_test_files} ${lite_test_proto_files})
target_link_libraries(protobuf-lite-test-common libprotobuf-lite GTest::gmock)

set(common_test_files
  ${common_lite_test_files}
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/mock_code_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_test_util.inc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/reflection_tester.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/testing/file.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/testing/googletest.cc
)

add_library(protobuf-test-common STATIC
  ${common_test_files} ${tests_proto_files})
target_link_libraries(protobuf-test-common libprotobuf GTest::gmock)

set(tests_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/any_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arena_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arenastring_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arenaz_sampler_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/annotation_test_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/annotation_test_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/command_line_interface_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/bootstrap_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/message_size_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/metadata_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/move_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/plugin_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/unittest.inc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_bootstrap_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_generator_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/importer_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/doc_comment_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/plugin_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_helpers_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/parser_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/plugin_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/ruby/ruby_generator_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor_database_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/drop_unknown_fields_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/dynamic_message_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/extension_set_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_reflection_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_tctable_lite_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/inlined_string_field_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/coded_stream_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/io_win32_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/printer_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/tokenizer_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/zero_copy_stream_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_field_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_test.inc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/message_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/message_unittest.inc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/no_field_presence_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/preserve_unknown_enum_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/proto3_arena_lite_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/proto3_arena_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/proto3_lite_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/proto3_lite_unittest.inc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/reflection_ops_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/repeated_field_reflection_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/repeated_field_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/bytestream_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/common_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/int128_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/status_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/statusor_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/stringpiece_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/stringprintf_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/structurally_valid_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/strutil_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/template_util_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/time_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/text_format_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unknown_field_set_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/delimited_message_util_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/field_comparator_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/field_mask_util_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/default_value_objectwriter_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/json_objectwriter_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/json_stream_parser_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/protostream_objectsource_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/protostream_objectwriter_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/type_info_test_helper.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/json_util_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/message_differencer_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/time_util_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/type_resolver_util_test.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/well_known_types_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/wire_format_unittest.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/wire_format_unittest.inc
)

if(protobuf_ABSOLUTE_TEST_PLUGIN_PATH)
  add_compile_options(-DGOOGLE_PROTOBUF_TEST_PLUGIN_PATH="$<TARGET_FILE:test_plugin>")
endif()

if(MINGW)
  set_source_files_properties(${tests_files} PROPERTIES COMPILE_FLAGS "-Wno-narrowing")

  # required for tests on MinGW Win64
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,16777216")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
  endif()

endif()

if(protobuf_TEST_XML_OUTDIR)
  if(NOT "${protobuf_TEST_XML_OUTDIR}" MATCHES "[/\\]$")
    string(APPEND protobuf_TEST_XML_OUTDIR "/")
  endif()
  set(protobuf_GTEST_ARGS "--gtest_output=xml:${protobuf_TEST_XML_OUTDIR}")
else()
  set(protobuf_GTEST_ARGS)
endif()

add_executable(tests ${tests_files})
if (MSVC)
  target_compile_options(tests PRIVATE
    /wd4146 # unary minus operator applied to unsigned type, result still unsigned
  )
endif()
target_link_libraries(tests protobuf-lite-test-common protobuf-test-common libprotoc libprotobuf GTest::gmock_main)

set(test_plugin_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/mock_code_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/test_plugin.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/testing/file.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/testing/file.h
)

add_executable(test_plugin ${test_plugin_files})
target_link_libraries(test_plugin libprotoc libprotobuf GTest::gmock)

set(lite_test_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/lite_unittest.cc
)
add_executable(lite-test ${lite_test_files})
target_link_libraries(lite-test protobuf-lite-test-common libprotobuf-lite GTest::gmock_main)

add_test(NAME lite-test
  COMMAND lite-test ${protobuf_GTEST_ARGS})

set(lite_arena_test_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/lite_arena_unittest.cc
)
add_executable(lite-arena-test ${lite_arena_test_files})
target_link_libraries(lite-arena-test protobuf-lite-test-common libprotobuf-lite GTest::gmock_main)

add_test(NAME lite-arena-test
  COMMAND lite-arena-test ${protobuf_GTEST_ARGS})

add_custom_target(check
  COMMAND tests
  DEPENDS tests test_plugin
  WORKING_DIRECTORY ${protobuf_SOURCE_DIR})

add_test(NAME check
  COMMAND tests ${protobuf_GTEST_ARGS}
  WORKING_DIRECTORY "${protobuf_SOURCE_DIR}")
