set(protoc_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/main.cc
)

add_executable(protoc ${protoc_files} ${protobuf_version_rc_file})
target_link_libraries(protoc
  libprotoc
  libprotobuf
)
add_executable(protobuf::protoc ALIAS protoc)

set_target_properties(protoc PROPERTIES
    VERSION ${protobuf_VERSION})
