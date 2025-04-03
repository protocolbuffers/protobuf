set(protoc_gen_rust_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/rust/main.cc
)

add_executable(protoc-gen-rust ${protoc_gen_rust_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-rust libprotoc)
add_executable(protobuf::protoc_gen_rust ALIAS protoc-gen-rust)

set_target_properties(protoc-gen-rust PROPERTIES
    VERSION ${protobuf_VERSION})
