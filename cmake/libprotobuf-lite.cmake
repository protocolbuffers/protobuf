set(libprotobuf_lite_files
  ${protobuf_source_dir}/src/google/protobuf/arena.cc
  ${protobuf_source_dir}/src/google/protobuf/arenastring.cc
  ${protobuf_source_dir}/src/google/protobuf/extension_set.cc
  ${protobuf_source_dir}/src/google/protobuf/generated_message_util.cc
  ${protobuf_source_dir}/src/google/protobuf/io/coded_stream.cc
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream.cc
  ${protobuf_source_dir}/src/google/protobuf/io/zero_copy_stream_impl_lite.cc
  ${protobuf_source_dir}/src/google/protobuf/message_lite.cc
  ${protobuf_source_dir}/src/google/protobuf/repeated_field.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/atomicops_internals_x86_gcc.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/atomicops_internals_x86_msvc.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/bytestream.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/common.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/int128.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/once.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/status.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/statusor.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/stringpiece.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/stringprintf.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/structurally_valid.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/strutil.cc
  ${protobuf_source_dir}/src/google/protobuf/stubs/time.cc
  ${protobuf_source_dir}/src/google/protobuf/wire_format_lite.cc
)

add_library(libprotobuf-lite ${protobuf_SHARED_OR_STATIC}
  ${libprotobuf_lite_files})
target_link_libraries(libprotobuf-lite ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(libprotobuf-lite PUBLIC ${protobuf_source_dir}/src)
if(MSVC AND protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libprotobuf-lite
    PUBLIC  PROTOBUF_USE_DLLS
    PRIVATE LIBPROTOBUF_EXPORTS)
endif()
set_target_properties(libprotobuf-lite PROPERTIES
    OUTPUT_NAME ${LIB_PREFIX}protobuf-lite
    DEBUG_POSTFIX "${protobuf_DEBUG_POSTFIX}")
