set(protoc_gen_cpp_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/cpp/main.cc
)

add_executable(protoc-gen-cpp ${protoc_gen_cpp_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-cpp libprotoc)
add_executable(protobuf::protoc_gen_cpp ALIAS protoc-gen-cpp)

set_target_properties(protoc-gen-cpp PROPERTIES
    VERSION ${protobuf_VERSION})
