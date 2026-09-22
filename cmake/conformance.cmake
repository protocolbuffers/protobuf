# Don't run jsoncpp tests.
set(JSONCPP_WITH_TESTS OFF)

include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)
include(${protobuf_SOURCE_DIR}/cmake/protobuf-configure-target.cmake)

if (NOT TARGET jsoncpp_lib AND NOT TARGET jsoncpp_static)
  if (NOT protobuf_FORCE_FETCH_DEPENDENCIES)
    find_package(jsoncpp)
  endif()

  # Fallback to fetching jsoncpp from github if it's not found locally.
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

if (NOT TARGET jsoncpp_lib AND NOT TARGET jsoncpp_static)
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
    ${protobuf_BINARY_DIR}/conformance/conformance_result.pb.h
    ${protobuf_BINARY_DIR}/conformance/conformance_result.pb.cc
    ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition2023.pb.h
    ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition2023.pb.cc
    ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition_unstable.pb.h
    ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition_unstable.pb.cc
  DEPENDS ${protobuf_PROTOC_EXE}
    ${protobuf_SOURCE_DIR}/conformance/conformance.proto
    ${protobuf_SOURCE_DIR}/conformance/conformance_result.proto
    ${protobuf_SOURCE_DIR}/conformance/test_protos/test_messages_edition2023.proto
    ${protobuf_SOURCE_DIR}/conformance/test_protos/test_messages_edition_unstable.proto
  COMMAND ${protobuf_PROTOC_EXE}
      ${protobuf_SOURCE_DIR}/conformance/conformance.proto
      ${protobuf_SOURCE_DIR}/conformance/conformance_result.proto
      ${protobuf_SOURCE_DIR}/conformance/test_protos/test_messages_edition2023.proto
      ${protobuf_SOURCE_DIR}/conformance/test_protos/test_messages_edition_unstable.proto
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
  ${protobuf_BINARY_DIR}/conformance/conformance_result.pb.h
  ${protobuf_BINARY_DIR}/conformance/conformance_result.pb.cc
  ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition2023.pb.h
  ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition2023.pb.cc
  ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition_unstable.pb.h
  ${protobuf_BINARY_DIR}/conformance/test_protos/test_messages_edition_unstable.pb.cc
  ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto3_editions.pb.h
  ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto3_editions.pb.cc
  ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto2_editions.pb.h
  ${protobuf_BINARY_DIR}/editions/golden/test_messages_proto2_editions.pb.cc
  ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto2.pb.h
  ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto2.pb.cc
  ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto3.pb.h
  ${protobuf_BINARY_DIR}/src/google/protobuf/test_messages_proto3.pb.cc
)
protobuf_configure_target(libconformance_common)
target_link_libraries(libconformance_common
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
)

add_executable(conformance_test_runner
  ${conformance_runner_srcs}
  ${conformance_runner_hdrs}
)
protobuf_configure_target(conformance_test_runner)

add_executable(conformance_cpp
  ${conformance_testee_srcs}
  ${conformance_testee_hdrs}
)
protobuf_configure_target(conformance_cpp)

target_include_directories(
  conformance_test_runner
  PUBLIC ${protobuf_SOURCE_DIR})

target_include_directories(
  conformance_cpp
  PUBLIC ${protobuf_SOURCE_DIR})

target_include_directories(conformance_test_runner PRIVATE ${ABSL_ROOT_DIR})
target_include_directories(conformance_cpp PRIVATE ${ABSL_ROOT_DIR})

# The runner hosts the gtest-based conformance suites (see
# conformance/conformance_test_main.cc), so it needs googletest even when
# protobuf_BUILD_TESTS is off.  gtest.cmake is a no-op if GTest::gmock already
# exists, and otherwise finds or fetches googletest the same way the unit tests
# do.
include(${protobuf_SOURCE_DIR}/cmake/gtest.cmake)

target_link_libraries(conformance_test_runner
  libconformance_common
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
  GTest::gmock
)
# The runner reads and sets gtest's flags (GTEST_FLAG_GET/GTEST_FLAG_SET), which
# are exported data symbols of the gtest library.  When googletest is built as
# a shared library, consumers must compile with GTEST_LINKED_AS_SHARED_LIBRARY
# so that those symbols are declared dllimport on Windows; googletest only
# attaches that definition to its installed (find_package) targets, not to the
# ones FetchContent builds in-tree, so set it here for either case.
if(TARGET GTest::gtest)
  get_target_property(_gtest_target_type GTest::gtest TYPE)
  if(_gtest_target_type STREQUAL "SHARED_LIBRARY")
    target_compile_definitions(conformance_test_runner
      PRIVATE GTEST_LINKED_AS_SHARED_LIBRARY=1)
  endif()
endif()

target_link_libraries(conformance_cpp
  libconformance_common
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
)

add_test(NAME conformance_cpp_test
  COMMAND $<TARGET_FILE:conformance_test_runner>
    --failure_list ${protobuf_SOURCE_DIR}/conformance/failure_list_cpp.txt
    --text_format_failure_list ${protobuf_SOURCE_DIR}/conformance/text_format_failure_list_cpp.txt
    --output_dir ${protobuf_TEST_XML_OUTDIR}
    --maximum_edition 2023
    $<TARGET_FILE:conformance_cpp>
  DEPENDS conformance_test_runner conformance_cpp)

set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Disable tests")

if(TARGET jsoncpp_static AND NOT BUILD_SHARED_LIBS)
  target_link_libraries(conformance_test_runner jsoncpp_static)
else()
  target_link_libraries(conformance_test_runner jsoncpp_lib)
endif()
