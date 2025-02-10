option(protobuf_REMOVE_INSTALLED_HEADERS
  "Remove local headers so that installed ones are used instead" OFF)

option(protobuf_ABSOLUTE_TEST_PLUGIN_PATH
  "Using absolute test_plugin path in tests" ON)
mark_as_advanced(protobuf_ABSOLUTE_TEST_PLUGIN_PATH)

include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)
include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)

set(lite_test_protos
  ${protobuf_lite_test_protos_files}
)

set(tests_protos
  ${protobuf_test_protos_files}
  ${compiler_test_protos_files}
  ${util_test_protos_files}
)

file(MAKE_DIRECTORY ${protobuf_BINARY_DIR}/src)

set(lite_test_proto_files)
foreach(proto_file ${lite_test_protos})
  protobuf_generate(
    PROTOS ${proto_file}
    LANGUAGE cpp
    OUT_VAR pb_generated_files
    IMPORT_DIRS ${protobuf_SOURCE_DIR}/src
  )
  set(lite_test_proto_files ${lite_test_proto_files} ${pb_generated_files})
endforeach(proto_file)

set(tests_proto_files)
foreach(proto_file ${tests_protos})
  protobuf_generate(
    PROTOS ${proto_file}
    LANGUAGE cpp
    OUT_VAR pb_generated_files
    IMPORT_DIRS ${protobuf_SOURCE_DIR}/src
  )
  set(tests_proto_files ${tests_proto_files} ${pb_generated_files})
endforeach(proto_file)

set(common_test_files
  ${test_util_hdrs}
  ${lite_test_util_srcs}
  ${test_util_srcs}
  ${common_test_hdrs}
  ${common_test_srcs}
)

set(tests_files
  ${protobuf_test_files}
  ${compiler_test_files}
  ${annotation_test_util_srcs}
  ${editions_test_files}
  ${io_test_files}
  ${util_test_files}
  ${stubs_test_files}
)

if(protobuf_ABSOLUTE_TEST_PLUGIN_PATH)
  add_compile_options(-DGOOGLE_PROTOBUF_FAKE_PLUGIN_PATH="$<TARGET_FILE:fake_plugin>")
  add_compile_options(-DGOOGLE_PROTOBUF_TEST_PLUGIN_PATH="$<TARGET_FILE:test_plugin>")
endif()

if(MINGW)
  set_source_files_properties(${tests_files} PROPERTIES COMPILE_FLAGS "-Wno-narrowing")

  # required for tests on MinGW Win64
  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--stack,16777216")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
  endif()

endif()

if(protobuf_TEST_XML_OUTDIR)
  if(NOT "${protobuf_TEST_XML_OUTDIR}" MATCHES "[/\\]$")
    string(APPEND protobuf_TEST_XML_OUTDIR "/")
  endif()
  set(protobuf_GTEST_ARGS "--gtest_output=xml:${protobuf_TEST_XML_OUTDIR}")
else()
  set(protobuf_GTEST_ARGS)
endif()

add_library(libtest_common STATIC
  ${tests_proto_files}
)
target_link_libraries(libtest_common
  ${protobuf_LIB_PROTOC}
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
  ${protobuf_ABSL_USED_TEST_TARGETS}
  GTest::gmock
)
if (MSVC)
  target_compile_options(libtest_common PRIVATE /bigobj)
endif ()

add_executable(tests ${tests_files} ${common_test_files})
if (MSVC)
  target_compile_options(tests PRIVATE
    /wd4146 # unary minus operator applied to unsigned type, result still unsigned
    /bigobj
  )
endif()
target_link_libraries(tests
  libtest_common
  libtest_common_lite
  ${protobuf_LIB_PROTOC}
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
  ${protobuf_ABSL_USED_TEST_TARGETS}
  GTest::gmock_main
)

add_executable(fake_plugin ${fake_plugin_files} ${common_test_files})
target_include_directories(fake_plugin PRIVATE ${ABSL_ROOT_DIR})
target_link_libraries(fake_plugin
  libtest_common
  libtest_common_lite
  ${protobuf_LIB_PROTOC}
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
  ${protobuf_ABSL_USED_TEST_TARGETS}
  GTest::gmock
)

add_executable(test_plugin ${test_plugin_files} ${common_test_files})
target_include_directories(test_plugin PRIVATE ${ABSL_ROOT_DIR})
target_link_libraries(test_plugin
  libtest_common
  libtest_common_lite
  ${protobuf_LIB_PROTOC}
  ${protobuf_LIB_PROTOBUF}
  ${protobuf_ABSL_USED_TARGETS}
  ${protobuf_ABSL_USED_TEST_TARGETS}
  GTest::gmock
)

add_library(libtest_common_lite STATIC
  ${lite_test_proto_files}
)
target_link_libraries(libtest_common_lite
  ${protobuf_LIB_PROTOBUF_LITE}
  ${protobuf_ABSL_USED_TARGETS}
  GTest::gmock
)

add_executable(lite-test
  ${protobuf_lite_test_files}
  ${lite_test_util_hdrs}
  ${lite_test_util_srcs}
)
target_link_libraries(lite-test
  libtest_common_lite
  ${protobuf_LIB_PROTOBUF_LITE}
  ${protobuf_ABSL_USED_TARGETS}
  ${protobuf_ABSL_USED_TEST_TARGETS}
  GTest::gmock_main
)

add_test(NAME lite-test
  COMMAND lite-test ${protobuf_GTEST_ARGS}
  WORKING_DIRECTORY ${protobuf_SOURCE_DIR})

add_custom_target(full-test
  COMMAND tests
  DEPENDS tests lite-test fake_plugin test_plugin
  WORKING_DIRECTORY ${protobuf_SOURCE_DIR})

add_test(NAME full-test
  COMMAND tests ${protobuf_GTEST_ARGS}
  WORKING_DIRECTORY ${protobuf_SOURCE_DIR})

if (protobuf_BUILD_LIBUPB)
  set(upb_test_proto_genfiles)
  foreach(proto_file ${upb_test_protos_files} ${descriptor_proto_proto_srcs})
    foreach(generator upb upbdefs upb_minitable)
      protobuf_generate(
        PROTOS ${proto_file}
        LANGUAGE ${generator}
        GENERATE_EXTENSIONS .${generator}.h .${generator}.c
        OUT_VAR pb_generated_files
        IMPORT_DIRS ${protobuf_SOURCE_DIR}/src
        IMPORT_DIRS ${protobuf_SOURCE_DIR}
        PLUGIN protoc-gen-${generator}=$<TARGET_FILE:protobuf::protoc-gen-${generator}>
        DEPENDENCIES $<TARGET_FILE:protobuf::protoc-gen-${generator}>
      )
      set(upb_test_proto_genfiles ${upb_test_proto_genfiles} ${pb_generated_files})
    endforeach()
  endforeach(proto_file)

  add_executable(upb-test
    ${upb_test_files}
    ${upb_test_proto_genfiles}
    ${upb_test_util_files})

  target_link_libraries(upb-test
    ${protobuf_LIB_PROTOBUF}
    ${protobuf_LIB_UPB}
    ${protobuf_ABSL_USED_TARGETS}
    ${protobuf_ABSL_USED_TEST_TARGETS}
    GTest::gmock_main)

  add_test(NAME upb-test
    COMMAND upb-test ${protobuf_GTEST_ARGS}
    WORKING_DIRECTORY ${protobuf_SOURCE_DIR})
endif()

# For test purposes, remove headers that should already be installed.  This
# prevents accidental conflicts and also version skew (since local headers take
# precedence over installed headers).
add_custom_target(save-installed-headers)
add_custom_target(remove-installed-headers)
add_custom_target(restore-installed-headers)

file(GLOB_RECURSE _local_hdrs
  "${PROJECT_SOURCE_DIR}/src/*.h"
  "${PROJECT_SOURCE_DIR}/src/*.inc"
)
file(GLOB_RECURSE _local_upb_hdrs
  "${PROJECT_SOURCE_DIR}/upb/*.h"
)

# Exclude test library headers.
list(APPEND _exclude_hdrs ${test_util_hdrs} ${lite_test_util_hdrs} ${common_test_hdrs}
  ${compiler_test_utils_hdrs} ${upb_test_util_files} ${libprotoc_hdrs})
foreach(_hdr ${_exclude_hdrs})
  list(REMOVE_ITEM _local_hdrs ${_hdr})
  list(REMOVE_ITEM _local_upb_hdrs ${_hdr})
endforeach()
list(APPEND _local_hdrs ${libprotoc_public_hdrs})

# Exclude the bootstrapping that are directly used by tests.
set(_exclude_hdrs
  "${protobuf_SOURCE_DIR}/src/google/protobuf/cpp_features.pb.h"
  "${protobuf_SOURCE_DIR}/src/google/protobuf/descriptor.pb.h"
  "${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/plugin.pb.h"
  "${protobuf_SOURCE_DIR}/src/google/protobuf/compiler/java/java_features.pb.h")
foreach(_hdr ${_exclude_hdrs})
  list(REMOVE_ITEM _local_hdrs ${_hdr})
endforeach()

foreach(_hdr ${_local_hdrs})
  string(REPLACE "${protobuf_SOURCE_DIR}/src" "" _file ${_hdr})
  set(_tmp_file "${CMAKE_BINARY_DIR}/tmp-install-test/${_file}")
  add_custom_command(TARGET remove-installed-headers PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E remove -f "${_hdr}")
  add_custom_command(TARGET save-installed-headers PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                        copy "${_hdr}" "${_tmp_file}" || true)
  add_custom_command(TARGET restore-installed-headers PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                        copy "${_tmp_file}" "${_hdr}")
endforeach()

foreach(_hdr ${_local_upb_hdrs})
  string(REPLACE "${protobuf_SOURCE_DIR}/upb" "" _file ${_hdr})
  set(_tmp_file "${CMAKE_BINARY_DIR}/tmp-install-test/${_file}")
  add_custom_command(TARGET remove-installed-headers PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E remove -f "${_hdr}")
  add_custom_command(TARGET save-installed-headers PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                        copy "${_hdr}" "${_tmp_file}" || true)
  add_custom_command(TARGET restore-installed-headers PRE_BUILD
                     COMMAND ${CMAKE_COMMAND} -E
                        copy "${_tmp_file}" "${_hdr}")
endforeach()

add_dependencies(remove-installed-headers save-installed-headers)
if(protobuf_REMOVE_INSTALLED_HEADERS)
  # Make sure we remove all the headers *before* any codegen occurs.
  add_dependencies(${protobuf_PROTOC_EXE} remove-installed-headers)
endif()
