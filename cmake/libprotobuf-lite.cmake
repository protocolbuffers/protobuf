set(libprotobuf_lite_files
  ${protobuf_source_dir}/src/google/protobuf/any_lite.cc
  ${protobuf_source_dir}/src/google/protobuf/arena.cc
  ${protobuf_source_dir}/src/google/protobuf/arenastring.cc
  ${protobuf_source_dir}/src/google/protobuf/extension_set.cc
  ${protobuf_source_dir}/src/google/protobuf/generated_enum_util.cc
  ${protobuf_source_dir}/src/google/protobuf/generated_message_table_driven_lite.cc
  ${protobuf_source_dir}/src/google/protobuf/generated_message_tctable_lite.cc
  ${protobuf_source_dir}/src/google/protobuf/generated_message_util.cc
  ${protobuf_source_dir}/src/google/protobuf/implicit_weak_message.cc
  ${protobuf_source_dir}/src/google/protobuf/inlined_string_field.cc
  ${protobuf_source_dir}/src/google/protobuf/io/coded_stream.cc
  ${protobuf_source_dir}/src/google/protobuf/io/io_win32.cc
  ${protobuf_source_dir}/src/google/protobuf/io/strtod.cc
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream.cc
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl.cc
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
  ${protobuf_source_dir}/src/google/protobuf/map.cc
  ${protobuf_source_dir}/src/google/protobuf/message_lite.cc
  ${protobuf_source_dir}/src/google/protobuf/parse_context.cc
  ${protobuf_source_dir}/src/google/protobuf/repeated_field.cc
  ${protobuf_source_dir}/src/google/protobuf/repeated_ptr_field.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/bytestream.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/common.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/int128.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/status.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/statusor.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/stringpiece.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/stringprintf.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/structurally_valid.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/strutil.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/time.cc
  ${protobuf_source_dir}/src/google/protobuf/wire_format_lite.cc
)

set(libprotobuf_lite_includes
  ${protobuf_source_dir}/src/google/protobuf/any.h
  ${protobuf_source_dir}/src/google/protobuf/arena.h
  ${protobuf_source_dir}/src/google/protobuf/arena_impl.h
  ${protobuf_source_dir}/src/google/protobuf/arenastring.h
  ${protobuf_source_dir}/src/google/protobuf/explicitly_constructed.h
  ${protobuf_source_dir}/src/google/protobuf/extension_set.h
  ${protobuf_source_dir}/src/google/protobuf/extension_set_inl.h
  ${protobuf_source_dir}/src/google/protobuf/generated_enum_util.h
  ${protobuf_source_dir}/src/google/protobuf/generated_message_table_driven.h
  ${protobuf_source_dir}/src/google/protobuf/generated_message_table_driven_lite.h
  ${protobuf_source_dir}/src/google/protobuf/generated_message_tctable_decl.h
  ${protobuf_source_dir}/src/google/protobuf/generated_message_tctable_impl.h
  ${protobuf_source_dir}/src/google/protobuf/generated_message_tctable_impl.inc
  ${protobuf_source_dir}/src/google/protobuf/generated_message_util.h
  ${protobuf_source_dir}/src/google/protobuf/has_bits.h
  ${protobuf_source_dir}/src/google/protobuf/implicit_weak_message.h
  ${protobuf_source_dir}/src/google/protobuf/inlined_string_field.h
  ${protobuf_source_dir}/src/google/protobuf/io/coded_stream.h
  ${protobuf_source_dir}/src/google/protobuf/io/io_win32.h
  ${protobuf_source_dir}/src/google/protobuf/io/strtod.h
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream.h
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl.h
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl_lite.h
  ${protobuf_source_dir}/src/google/protobuf/map.h
  ${protobuf_source_dir}/src/google/protobuf/map_entry_lite.h
  ${protobuf_source_dir}/src/google/protobuf/map_field_lite.h
  ${protobuf_source_dir}/src/google/protobuf/map_type_handler.h
  ${protobuf_source_dir}/src/google/protobuf/message_lite.h
  ${protobuf_source_dir}/src/google/protobuf/metadata_lite.h
  ${protobuf_source_dir}/src/google/protobuf/parse_context.h
  ${protobuf_source_dir}/src/google/protobuf/port.h
  ${protobuf_source_dir}/src/google/protobuf/repeated_field.h
  ${protobuf_source_dir}/src/google/protobuf/repeated_ptr_field.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/bytestream.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/callback.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/casts.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/common.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/hash.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/logging.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/macros.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/map_util.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/mutex.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/once.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/platform_macros.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/port.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/status.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/stl_util.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/stringpiece.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/strutil.h
  ${protobuf_source_dir}/src/google/protobuf/stubs/template_util.h
  ${protobuf_source_dir}/src/google/protobuf/wire_format_lite.h
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
set(libprotobuf_lite_rc_files
  ${CMAKE_CURRENT_BINARY_DIR}/version.rc
)
endif()

add_library(libprotobuf-lite ${protobuf_SHARED_OR_STATIC}
  ${libprotobuf_lite_files} ${libprotobuf_lite_includes} ${libprotobuf_lite_rc_files})
target_link_libraries(libprotobuf-lite ${CMAKE_THREAD_LIBS_INIT})
if(protobuf_LINK_LIBATOMIC)
  target_link_libraries(libprotobuf-lite atomic)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	target_link_libraries(libprotobuf-lite log)
endif()
target_include_directories(libprotobuf-lite PUBLIC ${protobuf_source_dir}/src)
if(MSVC AND protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libprotobuf-lite
    PUBLIC  PROTOBUF_USE_DLLS
    PRIVATE LIBPROTOBUF_EXPORTS)
endif()
set_target_properties(libprotobuf-lite PROPERTIES
    VERSION ${protobuf_VERSION}
    OUTPUT_NAME ${LIB_PREFIX}protobuf-lite
    DEBUG_POSTFIX "${protobuf_DEBUG_POSTFIX}")
add_library(protobuf::libprotobuf-lite ALIAS libprotobuf-lite)
