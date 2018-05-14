set(protoc_files
  ${protobuf_source_dir}/src/google/protobuf/compiler/main.cc
)

set(protoc_rc_files
  ${CMAKE_CURRENT_BINARY_DIR}/version.rc
)

add_executable(protoc ${protoc_files} ${protoc_rc_files})
target_link_libraries(protoc libprotobuf libprotoc)
add_executable(protobuf::protoc ALIAS protoc)

set_target_properties(protoc PROPERTIES
    VERSION ${protobuf_VERSION})
