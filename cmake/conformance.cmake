if (protobuf_JSONCPP_PROVIDER STREQUAL "module")
  if (NOT EXISTS "${protobuf_SOURCE_DIR}/third_party/jsoncpp/CMakeLists.txt")
    message(FATAL_ERROR
            "Cannot find third_party/jsoncpp directory that's needed to "
            "build conformance tests. If you use git, make sure you have cloned "
            "submodules:\n"
            "  git submodule update --init --recursive\n"
            "If instead you want to skip them, run cmake with:\n"
            "  cmake -Dprotobuf_BUILD_CONFORMANCE=OFF\n")
  endif()
elseif(protobuf_JSONCPP_PROVIDER STREQUAL "package")
  find_package(jsoncpp REQUIRED)
endif()

set(protoc_cpp_args)
if (protobuf_BUILD_SHARED_LIBS)
  set(protoc_cpp_args "dllexport_decl=PROTOBUF_TEST_EXPORTS:")
endif ()

add_custom_command(
  OUTPUT
    ${protobuf_SOURCE_DIR}/conformance/conformance.pb.h
    ${protobuf_SOURCE_DIR}/conformance/conformance.pb.cc
  DEPENDS ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/conformance/conformance.proto
  COMMAND ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/conformance/conformance.proto
      --proto_path=${protobuf_SOURCE_DIR}/conformance
      --cpp_out=${protoc_cpp_args}${protobuf_SOURCE_DIR}/conformance
)

add_custom_command(
  OUTPUT
    ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.pb.h
    ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.pb.cc
    ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.pb.h
    ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.pb.cc
  DEPENDS ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.proto
          ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.proto
  COMMAND ${protobuf_PROTOC_EXE} ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.proto
                 ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.proto
      --proto_path=${protobuf_SOURCE_DIR}/src
      --cpp_out=${protoc_cpp_args}${protobuf_SOURCE_DIR}/src
)

add_library(libconformance_common ${protobuf_SHARED_OR_STATIC}
  ${protobuf_SOURCE_DIR}/conformance/conformance.pb.h
  ${protobuf_SOURCE_DIR}/conformance/conformance.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.pb.cc
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.pb.h
  ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.pb.cc
)
target_link_libraries(libconformance_common
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
)
if(protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libconformance_common
    PUBLIC  PROTOBUF_USE_DLLS
    PRIVATE LIBPROTOBUF_TEST_EXPORTS)
endif()

add_executable(conformance_test_runner
  ${protobuf_SOURCE_DIR}/conformance/binary_json_conformance_suite.cc
  ${protobuf_SOURCE_DIR}/conformance/binary_json_conformance_suite.h
  ${protobuf_SOURCE_DIR}/conformance/conformance_test.cc
  ${protobuf_SOURCE_DIR}/conformance/conformance_test_runner.cc
  ${protobuf_SOURCE_DIR}/conformance/conformance_test_main.cc
  ${protobuf_SOURCE_DIR}/conformance/text_format_conformance_suite.cc
  ${protobuf_SOURCE_DIR}/conformance/text_format_conformance_suite.h
)

add_executable(conformance_cpp
  ${protobuf_SOURCE_DIR}/conformance/conformance_cpp.cc
)

target_include_directories(
  conformance_test_runner
  PUBLIC ${protobuf_SOURCE_DIR} ${protobuf_SOURCE_DIR}/conformance)

target_include_directories(
  conformance_cpp
  PUBLIC ${protobuf_SOURCE_DIR})

target_include_directories(conformance_test_runner PRIVATE ${ABSL_ROOT_DIR})
target_include_directories(conformance_cpp PRIVATE ${ABSL_ROOT_DIR})

target_link_libraries(conformance_test_runner
  libconformance_common
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
)
target_link_libraries(conformance_cpp
  libconformance_common
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
)

add_test(NAME conformance_cpp_test
  COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/conformance_test_runner
    --failure_list ${protobuf_SOURCE_DIR}/conformance/failure_list_cpp.txt
    --text_format_failure_list ${protobuf_SOURCE_DIR}/conformance/text_format_failure_list_cpp.txt
    --output_dir ${protobuf_TEST_XML_OUTDIR}
    ${CMAKE_CURRENT_BINARY_DIR}/conformance_cpp
  DEPENDS conformance_test_runner conformance_cpp)

set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Disable tests")
if(protobuf_JSONCPP_PROVIDER STREQUAL "module")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsoncpp third_party/jsoncpp)
  target_include_directories(conformance_test_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/jsoncpp/include)
  if(BUILD_SHARED_LIBS)
    target_link_libraries(conformance_test_runner jsoncpp_lib)
  else()
    target_link_libraries(conformance_test_runner jsoncpp_static)
  endif()
else()
  target_link_libraries(conformance_test_runner jsoncpp)
endif()
