set(protoc_gen_python_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/python/main.cc
)

add_executable(protoc-gen-python ${protoc_gen_python_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-python libprotoc)
add_executable(protobuf::protoc_gen_python ALIAS protoc-gen-python)

set_target_properties(protoc-gen-python PROPERTIES
    VERSION ${protobuf_VERSION})
