set(libprotoc_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/code_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/command_line_interface.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/enum.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/enum_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/extension.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/file.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/helpers.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/map_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/message.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/message_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/padding_optimizer.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/parse_function_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/primitive_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/service.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/string_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_doc_comment.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_enum.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_enum_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_field_base.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_helpers.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_map_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_message.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_message_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_primitive_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_reflection_class.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_repeated_enum_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_repeated_message_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_repeated_primitive_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_source_generator_base.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_wrapper_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/context.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/doc_comment.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/enum.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/enum_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/enum_field_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/enum_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/extension.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/extension_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/file.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/generator_factory.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/helpers.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/kotlin_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/map_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/map_field_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/message.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/message_builder.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/message_builder_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/message_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/message_field_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/message_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/name_resolver.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/primitive_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/primitive_field_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/service.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/shared_code_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/string_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/string_field_lite.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_enum.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_enum_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_extension.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_file.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_helpers.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_map_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_message.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_message_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_oneof.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_primitive_field.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/php/php_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/plugin.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/plugin.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/helpers.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/pyi_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/ruby/ruby_generator.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/subprocess.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/zip_writer.cc
)

set(libprotoc_headers
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/code_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/command_line_interface.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/cpp_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/file.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/helpers.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/names.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_doc_comment.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_names.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/csharp_options.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/java_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/kotlin_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/names.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/objectivec_helpers.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/php/php_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/plugin.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/pyi_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/python_generator.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/ruby/ruby_generator.h
)

add_library(libprotoc ${protobuf_SHARED_OR_STATIC}
  ${libprotoc_files} ${libprotoc_headers} ${protobuf_version_rc_file})
if(protobuf_HAVE_LD_VERSION_SCRIPT)
  if(${CMAKE_VERSION} VERSION_GREATER 3.13 OR ${CMAKE_VERSION} VERSION_EQUAL 3.13)
    target_link_options(libprotoc PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotoc.map)
  elseif(protobuf_BUILD_SHARED_LIBS)
    target_link_libraries(libprotoc PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotoc.map)
  endif()
  set_target_properties(libprotoc PROPERTIES
    LINK_DEPENDS ${protobuf_SOURCE_DIR}/src/libprotoc.map)
endif()
target_link_libraries(libprotoc PRIVATE libprotobuf)
if(protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libprotoc
    PUBLIC  PROTOBUF_USE_DLLS
    PRIVATE LIBPROTOC_EXPORTS)
endif()
set_target_properties(libprotoc PROPERTIES
    COMPILE_DEFINITIONS LIBPROTOC_EXPORTS
    VERSION ${protobuf_VERSION}
    SOVERSION 32
    OUTPUT_NAME ${LIB_PREFIX}protoc
    DEBUG_POSTFIX "${protobuf_DEBUG_POSTFIX}")
add_library(protobuf::libprotoc ALIAS libprotoc)
