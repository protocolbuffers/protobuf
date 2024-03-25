set(protoc_gen_csharp_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/csharp/main.cc
)

add_executable(protoc-gen-csharp ${protoc_gen_csharp_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-csharp libprotoc)
add_executable(protobuf::protoc_gen_csharp ALIAS protoc-gen-csharp)

set_target_properties(protoc-gen-csharp PROPERTIES
    VERSION ${protobuf_VERSION})
