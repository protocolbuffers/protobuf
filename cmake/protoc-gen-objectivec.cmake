set(protoc_gen_objectivec_files
  ${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/objectivec/main.cc
)

add_executable(protoc-gen-objectivec ${protoc_gen_objectivec_files} ${protobuf_version_rc_file})
target_link_libraries(protoc-gen-objectivec libprotoc)
add_executable(protobuf::protoc_gen_objectivec ALIAS protoc-gen-objectivec)

set_target_properties(protoc-gen-objectivec PROPERTIES
    VERSION ${protobuf_VERSION})
