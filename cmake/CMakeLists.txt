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
set(CMAKE_C_STANDARD 99)


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

include_directories(..)
include_directories(../cmake)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup -flat_namespace")
elseif(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id")
endif()

enable_testing()

add_library(port INTERFACE)
add_library(upb
  ../upb/decode.c
  ../upb/decode.int.h
  ../upb/encode.c
  ../upb/msg.c
  ../upb/msg.h
  ../upb/table.c
  ../upb/table.int.h
  ../upb/upb.c
  ../upb/upb.int.h
  ../upb/decode.h
  ../upb/encode.h
  ../upb/upb.h
  ../upb/upb.hpp)
target_link_libraries(upb
  fastdecode
  port
  /third_party/wyhash)
add_library(fastdecode
  ../upb/decode.int.h
  ../upb/decode_fast.c
  ../upb/decode_fast.h
  ../upb/msg.h
  ../upb/upb.int.h)
target_link_libraries(fastdecode
  port
  table)
add_library(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE)
target_link_libraries(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  table
  upb)
add_library(reflection
  ../upb/def.c
  ../upb/msg.h
  ../upb/reflection.c
  ../upb/def.h
  ../upb/def.hpp
  ../upb/reflection.h)
target_link_libraries(reflection
  descriptor_upb_proto
  port
  table
  upb)
add_library(textformat
  ../upb/text_encode.c
  ../upb/text_encode.h)
target_link_libraries(textformat
  port
  reflection)
add_library(json
  ../upb/json_decode.c
  ../upb/json_encode.c
  ../upb/json_decode.h
  ../upb/json_encode.h)
target_link_libraries(json
  port
  reflection
  upb)
add_library(table INTERFACE)
target_link_libraries(table INTERFACE
  port)
add_library(handlers
  ../upb/handlers.c
  ../upb/handlers-inl.h
  ../upb/sink.c
  ../upb/handlers.h
  ../upb/sink.h)
target_link_libraries(handlers
  port
  reflection
  table
  upb)
add_library(upb_pb
  ../upb/pb/compile_decoder.c
  ../upb/pb/decoder.c
  ../upb/pb/decoder.int.h
  ../upb/pb/encoder.c
  ../upb/pb/textprinter.c
  ../upb/pb/varint.c
  ../upb/pb/varint.int.h
  ../upb/pb/decoder.h
  ../upb/pb/encoder.h
  ../upb/pb/textprinter.h)
target_link_libraries(upb_pb
  descriptor_upb_proto
  handlers
  port
  reflection
  table
  upb)
add_library(upb_json
  ../cmake/upb/json/parser.c
  ../upb/json/printer.c
  ../upb/json/parser.h
  ../upb/json/printer.h)
target_link_libraries(upb_json
  upb
  upb_pb)
add_library(wyhash INTERFACE)


