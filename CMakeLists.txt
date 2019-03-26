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
  upb/sink.c
  upb/structs.int.h
  upb/table.c
  upb/table.int.h
  upb/upb.c
  google/protobuf/descriptor.upb.h
  upb/decode.h
  upb/def.h
  upb/encode.h
  upb/generated_util.h
  upb/handlers.h
  upb/msg.h
  upb/msgfactory.h
  upb/sink.h
  upb/upb.h)
add_library(upb_pb
  upb/pb/compile_decoder.c
  upb/pb/decoder.c
  upb/pb/decoder.int.h
  upb/pb/encoder.c
  upb/pb/textprinter.c
  upb/pb/varint.c
  upb/pb/varint.int.h
  upb/table.int.h
  upb/pb/decoder.h
  upb/pb/encoder.h
  upb/pb/textprinter.h)
target_link_libraries(upb_pb
  upb)
add_library(upb_json
  upb/json/parser.c
  upb/json/printer.c
  upb/json/parser.h
  upb/json/printer.h)
target_link_libraries(upb_json
  upb
  upb_pb)
add_library(upb_cc_bindings INTERFACE)
target_link_libraries(upb_cc_bindings INTERFACE
  upb)
add_library(upb_test
  tests/testmain.cc
  tests/test_util.h
  tests/upb_test.h)


