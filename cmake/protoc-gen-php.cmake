set(protoc_gen_php_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/php/main.cc
)

add_executable(protoc-gen-php ${protoc_gen_php_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-php libprotoc)
add_executable(protobuf::protoc_gen_php ALIAS protoc-gen-php)

set_target_properties(protoc-gen-php PROPERTIES
    VERSION ${protobuf_VERSION})
