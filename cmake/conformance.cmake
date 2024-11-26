# Don't run jsoncpp tests.
set(JSONCPP_WITH_TESTS OFF)

if (NOT TARGET jsoncpp_lib)
  if (NOT protobuf_FORCE_FETCH_DEPENDENCIES)
    find_package(jsoncpp)
  endif()

  # Fallback to fetching Googletest from github if it's not found locally.
  if (NOT jsoncpp_FOUND AND NOT protobuf_LOCAL_DEPENDENCIES_ONLY)
    include(${protobuf_SOURCE_DIR}/cmake/dependencies.cmake)
    message(STATUS "Fallback to downloading jsoncpp ${jsoncpp-version} from GitHub")

    include(FetchContent)
    FetchContent_Declare(
      jsoncpp
      GIT_REPOSITORY "https://github.com/open-source-parsers/jsoncpp.git"
      GIT_TAG "${jsoncpp-version}"
    )
    FetchContent_MakeAvailable(jsoncpp)
  endif()
endif()

if (NOT TARGET jsoncpp_lib)
  message(FATAL_ERROR
          "Cannot find jsoncpp dependency that's needed to build conformance tests.\n"
          "If instead you want to skip these tests, run cmake with:\n"
          "  cmake -Dprotobuf_BUILD_CONFORMANCE=OFF\n")
endif()

file(MAKE_DIRECTORY ${protobuf_BINARY_DIR}/conformance)

add_custom_command(
  OUTPUT
    ${protobuf_BINARY_DIR}/conformance/conformance.pb.h
    ${protobuf_BINARY_DIR}/conformance/conformance.pb.cc
    ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition2023.pb.h
    ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition2023.pb.cc
  DEPENDS ${protobuf_PROTOC_EXE}
    ${protobuf_SOURCE_DIR}/conformance/conformance.proto
    ${protobuf_SOURCE_DIR}/conformance/test_protos/test_messages_edition2023.proto
  COMMAND ${protobuf_PROTOC_EXE}
      ${protobuf_SOURCE_DIR}/conformance/conformance.proto
      ${protobuf_SOURCE_DIR}/conformance/test_protos/test_messages_edition2023.proto
      --proto_path=${protobuf_SOURCE_DIR}
      --cpp_out=${protobuf_BINARY_DIR}
)


add_custom_command(
  OUTPUT
    ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto3_editions.pb.h
    ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto3_editions.pb.cc
    ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto2_editions.pb.h
    ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto2_editions.pb.cc
  DEPENDS ${protobuf_PROTOC_EXE}
    ${protobuf_SOURCE_DIR}/editions/golden/test_messages_proto3_editions.proto
    ${protobuf_SOURCE_DIR}/editions/golden/test_messages_proto2_editions.proto
  COMMAND ${protobuf_PROTOC_EXE}
      ${protobuf_SOURCE_DIR}/editions/golden/test_messages_proto3_editions.proto
      ${protobuf_SOURCE_DIR}/editions/golden/test_messages_proto2_editions.proto
      --proto_path=${protobuf_SOURCE_DIR}
      --proto_path=${protobuf_SOURCE_DIR}/src
      --cpp_out=${protobuf_BINARY_DIR}
)

file(MAKE_DIRECTORY ${protobuf_BINARY_DIR}/src)

add_custom_command(
  OUTPUT
    ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto3.pb.h
    ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto3.pb.cc
    ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto2.pb.h
    ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto2.pb.cc
  DEPENDS ${protobuf_PROTOC_EXE}
          ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.proto
          ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.proto
  COMMAND ${protobuf_PROTOC_EXE}
              ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto3.proto
              ${protobuf_SOURCE_DIR}/src/google/protobuf/test_messages_proto2.proto
            --proto_path=${protobuf_SOURCE_DIR}/src
            --cpp_out=${protobuf_BINARY_DIR}/src
)

add_library(libconformance_common STATIC
  ${protobuf_BINARY_DIR}/conformance/conformance.pb.h
  ${protobuf_BINARY_DIR}/conformance/conformance.pb.cc
  ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition2023.pb.h
  ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition2023.pb.cc
  ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto3_editions.pb.h
  ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto3_editions.pb.cc
  ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto2_editions.pb.h
  ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto2_editions.pb.cc
  ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto2.pb.h
  ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto2.pb.cc
  ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto3.pb.h
  ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto3.pb.cc
)
target_link_libraries(libconformance_common
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
)

add_executable(conformance_test_runner
  ${protobuf_SOURCE_DIR}/conformance/binary_json_conformance_suite.cc
  ${protobuf_SOURCE_DIR}/conformance/binary_json_conformance_suite.h
  ${protobuf_SOURCE_DIR}/conformance/conformance_test.cc
  ${protobuf_SOURCE_DIR}/conformance/conformance_test_runner.cc
  ${protobuf_SOURCE_DIR}/conformance/conformance_test_main.cc
  ${protobuf_SOURCE_DIR}/conformance/text_format_conformance_suite.cc
  ${protobuf_SOURCE_DIR}/conformance/text_format_conformance_suite.h
  ${protobuf_SOURCE_DIR}/conformance/failure_list_trie_node.cc
  ${protobuf_SOURCE_DIR}/conformance/failure_list_trie_node.h
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
    --maximum_edition 2023
    $<TARGET_FILE:conformance_cpp>
  DEPENDS conformance_test_runner conformance_cpp)

set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Disable tests")

if(BUILD_SHARED_LIBS)
  target_link_libraries(conformance_test_runner jsoncpp_lib)
else()
  target_link_libraries(conformance_test_runner jsoncpp_static)
endif()
