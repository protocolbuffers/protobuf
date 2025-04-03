set(protoc_gen_java_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/main.cc
)

add_executable(protoc-gen-java ${protoc_gen_java_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-java libprotoc)
add_executable(protobuf::protoc_gen_java ALIAS protoc-gen-java)

set_target_properties(protoc-gen-java PROPERTIES
    VERSION ${protobuf_VERSION})
