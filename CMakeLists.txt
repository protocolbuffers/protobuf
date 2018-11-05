# This file was generated from BUILD using tools/make_cmakelists.py.

cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.12)
endif()

cmake_minimum_required (VERSION 3.0)
cmake_policy(SET CMP0048 NEW)

project(upb)


# Prevent CMake from setting -rdynamic on Linux (!!).
SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# Set default build type.
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()

# When using Ninja, compiler output won't be colorized without this.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-fdiagnostics-color=always SUPPORTS_COLOR_ALWAYS)
if(SUPPORTS_COLOR_ALWAYS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
endif()

# Implement ASAN/UBSAN options
if(UPB_ENABLE_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()

if(UPB_ENABLE_UBSAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()

include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup -flat_namespace")
elseif(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id")
endif()

enable_testing()

add_library(upb
  google/protobuf/descriptor.upb.c
  upb/decode.c
  upb/def.c
  upb/encode.c
  upb/handlers.c
  upb/handlers-inl.h
  upb/msg.c
  upb/msgfactory.c
  upb/port_def.inc
  upb/port_undef.inc
  upb/refcounted.c
  upb/sink.c
  upb/structdefs.int.h
  upb/structs.int.h
  upb/table.c
  upb/table.int.h
  upb/upb.c
  google/protobuf/descriptor.upb.h
  upb/decode.h
  upb/def.h
  upb/encode.h
  upb/handlers.h
  upb/msg.h
  upb/msgfactory.h
  upb/refcounted.h
  upb/sink.h
  upb/upb.h)
add_library(upb_descriptor
  upb/descriptor/descriptor.upbdefs.c
  upb/descriptor/reader.c
  upb/descriptor/descriptor.upbdefs.h
  upb/descriptor/reader.h)
target_link_libraries(upb_descriptor
  upb)
add_library(upb_pb
  upb/pb/compile_decoder.c
  upb/pb/decoder.c
  upb/pb/decoder.int.h
  upb/pb/encoder.c
  upb/pb/glue.c
  upb/pb/textprinter.c
  upb/pb/varint.c
  upb/pb/varint.int.h
  upb/pb/decoder.h
  upb/pb/encoder.h
  upb/pb/glue.h
  upb/pb/textprinter.h)
target_link_libraries(upb_pb
  upb
  upb_descriptor)
add_library(upb_json
  upb/json/parser.c
  upb/json/printer.c
  upb/json/parser.h
  upb/json/printer.h)
target_link_libraries(upb_json
  upb)
add_library(upb_cc_bindings
  upb/bindings/stdc++/string.h)
target_link_libraries(upb_cc_bindings
  upb)
set_target_properties(upb_cc_bindings PROPERTIES LINKER_LANGUAGE CXX)
add_library(upb_test
  tests/testmain.cc
  tests/test_util.h
  tests/upb_test.h)
add_executable(test_varint
  tests/pb/test_varint.c)
add_test(NAME test_varint COMMAND test_varint)
target_link_libraries(test_varint
  upb_pb
  upb_test)
add_executable(test_def
  tests/test_def.c)
add_test(NAME test_def COMMAND test_def)
target_link_libraries(test_def
  upb_pb
  upb_test)
add_executable(test_handlers
  tests/test_handlers.c)
add_test(NAME test_handlers COMMAND test_handlers)
target_link_libraries(test_handlers
  upb_pb
  upb_test)
add_executable(test_decoder
  tests/pb/test_decoder.cc)
add_test(NAME test_decoder COMMAND test_decoder)
target_link_libraries(test_decoder
  upb_pb
  upb_test)
add_executable(test_encoder
  tests/pb/test_encoder.cc)
add_test(NAME test_encoder COMMAND test_encoder)
add_custom_command(
    TARGET test_encoder POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/upb/descriptor/descriptor.pb
            ${CMAKE_CURRENT_BINARY_DIR}/upb/descriptor/descriptor.pb)
target_link_libraries(test_encoder
  upb_cc_bindings
  upb_pb
  upb_test)
add_executable(test_cpp
  tests/test_cpp.cc)
add_test(NAME test_cpp COMMAND test_cpp)
target_link_libraries(test_cpp
  upb
  upb_descriptor
  upb_pb
  upb_test)
add_executable(test_table
  tests/test_table.cc)
add_test(NAME test_table COMMAND test_table)
target_link_libraries(test_table
  upb
  upb_test)
add_executable(test_json
  tests/json/test.upbdefs.c
  tests/json/test.upbdefs.h
  tests/json/test_json.cc)
add_test(NAME test_json COMMAND test_json)
target_link_libraries(test_json
  upb_json
  upb_test)


