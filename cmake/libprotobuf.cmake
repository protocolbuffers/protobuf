set(libprotobuf_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/any.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/any.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/api.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/importer.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/parser.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor_database.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/duration.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/dynamic_message.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/empty.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/extension_set_heavy.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/field_mask.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_bases.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_reflection.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_tctable_full.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/gzip_stream.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/printer.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/tokenizer.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/message.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/reflection_ops.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/service.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/source_context.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/struct.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/stubs/substitute.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/text_format.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/timestamp.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/type.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unknown_field_set.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/delimited_message_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/field_comparator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/field_mask_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/datapiece.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/default_value_objectwriter.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/error_listener.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/field_mask_utility.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/json_escaping.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/json_objectwriter.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/json_stream_parser.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/object_writer.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/proto_writer.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/protostream_objectsource.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/protostream_objectwriter.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/type_info.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/internal/utility.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/json_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/message_differencer.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/time_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/type_resolver_util.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/wire_format.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/wrappers.pb.cc
)

set(libprotobuf_includes
  ${protobuf_SOURCE_DIR}/src/google/protobuf/any.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/api.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/importer.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/parser.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor_database.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/duration.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/dynamic_message.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/empty.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/field_access_listener.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/field_mask.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_enum_reflection.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_bases.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/generated_message_reflection.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/gzip_stream.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/printer.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/io/tokenizer.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_entry.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_field.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/map_field_inl.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/message.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/metadata.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/reflection.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/reflection_internal.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/reflection_ops.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/service.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/source_context.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/struct.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/text_format.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/timestamp.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/type.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/unknown_field_set.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/delimited_message_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/field_comparator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/field_mask_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/json_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/message_differencer.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/time_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/type_resolver.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/util/type_resolver_util.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/wire_format.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/wrappers.pb.h
)

add_library(libprotobuf ${protobuf_SHARED_OR_STATIC}
  ${libprotobuf_lite_files} ${libprotobuf_files} ${libprotobuf_includes} ${protobuf_version_rc_file})
if(protobuf_HAVE_LD_VERSION_SCRIPT)
  if(${CMAKE_VERSION} VERSION_GREATER 3.13 OR ${CMAKE_VERSION} VERSION_EQUAL 3.13)
    target_link_options(libprotobuf PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotobuf.map)
  elseif(protobuf_BUILD_SHARED_LIBS)
    target_link_libraries(libprotobuf PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotobuf.map)
  endif()
  set_target_properties(libprotobuf PROPERTIES
    LINK_DEPENDS ${protobuf_SOURCE_DIR}/src/libprotobuf.map)
endif()
target_link_libraries(libprotobuf PRIVATE ${CMAKE_THREAD_LIBS_INIT})
if(protobuf_WITH_ZLIB)
  target_link_libraries(libprotobuf PRIVATE ${ZLIB_LIBRARIES})
endif()
if(protobuf_LINK_LIBATOMIC)
  target_link_libraries(libprotobuf PRIVATE atomic)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  target_link_libraries(libprotobuf PRIVATE log)
endif()
target_include_directories(libprotobuf PUBLIC ${protobuf_SOURCE_DIR}/src)
if(protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libprotobuf
    PUBLIC  PROTOBUF_USE_DLLS
    PRIVATE LIBPROTOBUF_EXPORTS)
endif()
set_target_properties(libprotobuf PROPERTIES
    VERSION ${protobuf_VERSION}
    SOVERSION 32
    OUTPUT_NAME ${LIB_PREFIX}protobuf
    DEBUG_POSTFIX "${protobuf_DEBUG_POSTFIX}")
add_library(protobuf::libprotobuf ALIAS libprotobuf)
