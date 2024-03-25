set(protoc_gen_pyi_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/pyi_main.cc
)

add_executable(protoc-gen-pyi ${protoc_gen_pyi_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-pyi libprotoc)
add_executable(protobuf::protoc_gen_pyi ALIAS protoc-gen-pyi)

set_target_properties(protoc-gen-pyi PROPERTIES
    VERSION ${protobuf_VERSION})
