set(protoc_files
  ${protobuf_source_dir}/src/google/protobuf/compiler/main.cc
)

add_executable(protoc ${protoc_files} ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
target_link_libraries(protoc libprotobuf libprotoc)
add_executable(protobuf::protoc ALIAS protoc)

set_target_properties(protoc PROPERTIES
    VERSION ${protobuf_VERSION})
