
add_custom_command(
  OUTPUT ${protobuf_SOURCE_DIR}/conformance/conformance.pb.cc
  DEPENDS ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/conformance/conformance.proto
  COMMAND ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/conformance/conformance.proto
      --proto_path=${protobuf_SOURCE_DIR}/conformance
      --cpp_out=${protobuf_SOURCE_DIR}/conformance
)

add_custom_command(
  OUTPUT ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.pb.cc
         ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.pb.cc
  DEPENDS ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.proto
          ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.proto
  COMMAND ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.proto
                 ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.proto
      --proto_path=${protobuf_SOURCE_DIR}/src
      --cpp_out=${protobuf_SOURCE_DIR}/src
)

add_executable(conformance_test_runner
  ${protobuf_SOURCE_DIR}/conformance/binary_json_conformance_suite.cc
  ${protobuf_SOURCE_DIR}/conformance/binary_json_conformance_suite.h
  ${protobuf_SOURCE_DIR}/conformance/conformance.pb.cc
  ${protobuf_SOURCE_DIR}/conformance/conformance_test.cc
  ${protobuf_SOURCE_DIR}/conformance/conformance_test_runner.cc
  ${protobuf_SOURCE_DIR}/conformance/third_party/jsoncpp/json.h
  ${protobuf_SOURCE_DIR}/conformance/third_party/jsoncpp/jsoncpp.cpp
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.pb.cc
)

add_executable(conformance_cpp
  ${protobuf_SOURCE_DIR}/conformance/conformance.pb.cc
  ${protobuf_SOURCE_DIR}/conformance/conformance_cpp.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.pb.cc
)

target_include_directories(
  conformance_test_runner
  PUBLIC ${protobuf_SOURCE_DIR}/conformance)

target_include_directories(
  conformance_cpp
  PUBLIC ${protobuf_SOURCE_DIR}/conformance)

target_link_libraries(conformance_test_runner libprotobuf)
target_link_libraries(conformance_cpp libprotobuf)
