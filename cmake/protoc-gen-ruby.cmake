set(protoc_gen_ruby_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/ruby/main.cc
)

add_executable(protoc-gen-ruby ${protoc_gen_ruby_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-ruby libprotoc)
add_executable(protobuf::protoc_gen_ruby ALIAS protoc-gen-ruby)

set_target_properties(protoc-gen-ruby PROPERTIES
    VERSION ${protobuf_VERSION})
