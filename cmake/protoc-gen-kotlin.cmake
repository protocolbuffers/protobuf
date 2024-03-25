set(protoc_gen_kotlin_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/kotlin_main.cc
)

add_executable(protoc-gen-kotlin ${protoc_gen_kotlin_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-kotlin libprotoc)
add_executable(protobuf::protoc_gen_kotlin ALIAS protoc-gen-kotlin)

set_target_properties(protoc-gen-kotlin PROPERTIES
    VERSION ${protobuf_VERSION})
