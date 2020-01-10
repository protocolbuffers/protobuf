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
include_directories(generated_for_cmake)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup -flat_namespace")
elseif(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id")
endif()

enable_testing()

add_library(port
  upb/port.c)
add_library(upb
  upb/decode.c
  upb/encode.c
  upb/generated_util.h
  upb/msg.c
  upb/msg.h
  upb/table.c
  upb/table.int.h
  upb/upb.c
  upb/decode.h
  upb/encode.h
  upb/upb.h)
target_link_libraries(upb
  port)
add_library(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE)
target_link_libraries(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  upb)
add_library(reflection
  upb/def.c
  upb/msgfactory.c
  upb/def.h
  upb/msgfactory.h)
target_link_libraries(reflection
  descriptor_upbproto
  port
  table
  upb)
add_library(table INTERFACE)
target_link_libraries(table INTERFACE
  port
  upb)
add_library(legacy_msg_reflection
  upb/msg.h
  upb/legacy_msg_reflection.c
  upb/legacy_msg_reflection.h)
target_link_libraries(legacy_msg_reflection
  port
  table
  upb)
add_library(handlers
  upb/handlers.c
  upb/handlers-inl.h
  upb/sink.c
  upb/handlers.h
  upb/sink.h)
target_link_libraries(handlers
  reflection
  port
  table
  upb)
add_library(upb_pb
  upb/pb/compile_decoder.c
  upb/pb/decoder.c
  upb/pb/decoder.int.h
  upb/pb/encoder.c
  upb/pb/textprinter.c
  upb/pb/varint.c
  upb/pb/varint.int.h
  upb/pb/decoder.h
  upb/pb/encoder.h
  upb/pb/textprinter.h)
target_link_libraries(upb_pb
  descriptor_upbproto
  handlers
  reflection
  port
  table
  upb)
add_library(upb_json
  generated_for_cmake/upb/json/parser.c
  upb/json/printer.c
  upb/json/parser.h
  upb/json/printer.h)
target_link_libraries(upb_json
  upb
  upb_pb)
add_library(upb_cc_bindings INTERFACE)
target_link_libraries(upb_cc_bindings INTERFACE
  descriptor_upbproto
  handlers
  port
  upb)
add_library(upb_test
  tests/testmain.cc
  tests/test_util.h
  tests/upb_test.h)
target_link_libraries(upb_test
  handlers
  port
  upb)


