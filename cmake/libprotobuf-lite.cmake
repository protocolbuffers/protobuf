set(libprotobuf_lite_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/any_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arena.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arenastring.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arenaz_sampler.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/extension_set.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_enum_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_tctable_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/implicit_weak_message.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/inlined_string_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/coded_stream.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/io_win32.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/strtod.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/zero_copy_stream.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/zero_copy_stream_impl.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/message_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/parse_context.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/repeated_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/repeated_ptr_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/bytestream.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/common.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/int128.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/status.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/statusor.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/stringpiece.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/stringprintf.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/structurally_valid.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/strutil.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/time.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/wire_format_lite.cc
)

set(libprotobuf_lite_includes
  ${protobuf_SOURCE_DIR}/src/google/protobuf/any.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arena.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arena_impl.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arenastring.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/arenaz_sampler.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/endian.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/explicitly_constructed.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/extension_set.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/extension_set_inl.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_enum_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_tctable_decl.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_tctable_impl.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/has_bits.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/implicit_weak_message.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/inlined_string_field.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/coded_stream.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/io_win32.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/strtod.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/zero_copy_stream.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/zero_copy_stream_impl.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/zero_copy_stream_impl_lite.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_entry_lite.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_field_lite.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_type_handler.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/message_lite.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/metadata_lite.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/parse_context.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/port.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/repeated_field.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/repeated_ptr_field.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/bytestream.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/callback.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/casts.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/common.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/hash.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/logging.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/macros.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/map_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/mutex.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/once.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/platform_macros.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/port.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/status.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/stl_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/stringpiece.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/strutil.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/template_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/wire_format_lite.h
)

add_library(libprotobuf-lite ${protobuf_SHARED_OR_STATIC}
  ${libprotobuf_lite_files} ${libprotobuf_lite_includes} ${protobuf_version_rc_file})
if(protobuf_HAVE_LD_VERSION_SCRIPT)
  if(${CMAKE_VERSION} VERSION_GREATER 3.13 OR ${CMAKE_VERSION} VERSION_EQUAL 3.13)
    target_link_options(libprotobuf-lite PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotobuf-lite.map)
  elseif(protobuf_BUILD_SHARED_LIBS)
    target_link_libraries(libprotobuf-lite PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotobuf-lite.map)
  endif()
  set_target_properties(libprotobuf-lite PROPERTIES
    LINK_DEPENDS ${protobuf_SOURCE_DIR}/src/libprotobuf-lite.map)
endif()
target_link_libraries(libprotobuf-lite PRIVATE ${CMAKE_THREAD_LIBS_INIT})
if(protobuf_LINK_LIBATOMIC)
  target_link_libraries(libprotobuf-lite PRIVATE atomic)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  target_link_libraries(libprotobuf-lite PRIVATE log)
endif()
target_include_directories(libprotobuf-lite PUBLIC ${protobuf_SOURCE_DIR}/src)
if(protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libprotobuf-lite
    PUBLIC  PROTOBUF_USE_DLLS
    PRIVATE LIBPROTOBUF_EXPORTS)
endif()
set_target_properties(libprotobuf-lite PROPERTIES
    VERSION ${protobuf_VERSION}
    SOVERSION 32
    OUTPUT_NAME ${LIB_PREFIX}protobuf-lite
    DEBUG_POSTFIX "${protobuf_DEBUG_POSTFIX}")
add_library(protobuf::libprotobuf-lite ALIAS libprotobuf-lite)
