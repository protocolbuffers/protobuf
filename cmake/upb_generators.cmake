include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)

set(bootstrap_cmake_dir ${protobuf_SOURCE_DIR}/upb_generator/cmake)
set(bootstrap_sources
  ${bootstrap_cmake_dir}/google/protobuf/compiler/plugin.upb.h
  ${bootstrap_cmake_dir}/google/protobuf/compiler/plugin.upb_minitable.h
  ${bootstrap_cmake_dir}/google/protobuf/compiler/plugin.upb_minitable.c
)

foreach(generator upb upbdefs upb_minitable)
  add_executable(protoc-gen-${generator}
    ${protoc-gen-${generator}_srcs}
    ${protoc-gen-${generator}_hdrs}
    ${bootstrap_sources}
    ${protobuf_version_rc_file}
  )
  target_include_directories(protoc-gen-${generator} PRIVATE ${bootstrap_cmake_dir})
  target_link_libraries(protoc-gen-${generator}
    utf8_validity
    ${protobuf_LIB_UPB}
    ${protobuf_ABSL_USED_TARGETS}
  )
  set_target_properties(protoc-gen-${generator} PROPERTIES VERSION ${protobuf_VERSION})
  add_executable(protobuf::protoc-gen-${generator} ALIAS protoc-gen-${generator})
endforeach()
