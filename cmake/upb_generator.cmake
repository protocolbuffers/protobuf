include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)

add_executable(protoc-gen-upb ${protoc-gen-upb_srcs} ${protoc-gen-upb_hdrs} ${protobuf_version_rc_file})
target_include_directories(protoc-gen-upb PUBLIC ${protobuf_SOURCE_DIR}/upb_generator/stage1)
target_link_libraries(protoc-gen-upb
  libprotoc
  libprotobuf
  libupb
  ${protobuf_ABSL_USED_TARGETS}
)
set_target_properties(protoc-gen-upb PROPERTIES
    VERSION ${protobuf_VERSION})

add_executable(protoc-gen-upbdefs ${protoc-gen-upbdefs_srcs} ${protoc-gen-upbdefs_hdrs} ${protobuf_version_rc_file})
target_include_directories(protoc-gen-upbdefs PUBLIC ${protobuf_SOURCE_DIR}/upb_generator/stage1)
target_link_libraries(protoc-gen-upbdefs
  libprotoc
  libprotobuf
  libupb
  ${protobuf_ABSL_USED_TARGETS}
)
set_target_properties(protoc-gen-upbdefs PROPERTIES
    VERSION ${protobuf_VERSION})

add_executable(protoc-gen-upb_minitable ${protoc-gen-upb_minitable_srcs} ${protoc-gen-upb_minitable_hdrs} ${protobuf_version_rc_file})
target_include_directories(protoc-gen-upb_minitable PUBLIC ${protobuf_SOURCE_DIR}/upb_generator/stage1)
target_link_libraries(protoc-gen-upb_minitable
  libprotoc
  libprotobuf
  libupb
  ${protobuf_ABSL_USED_TARGETS}
)
set_target_properties(protoc-gen-upb_minitable PROPERTIES
    VERSION ${protobuf_VERSION})
