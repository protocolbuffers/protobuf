
cmake_minimum_required (VERSION 3.0)
cmake_policy(SET CMP0048 NEW)
project (upb)

# Options we define for users.
option(UPB_ENABLE_ASAN "Enable address sanitizer." OFF)
option(UPB_ENABLE_UBSAN "Enable undefined behavior sanitizer." OFF)

# Set default build type.
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()

# Check out Git submodules.
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.gitmodules")
  execute_process (COMMAND git submodule update --init --recursive
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(protobuf_BUILD_TESTS OFF CACHE BOOL "enable tests for proto2" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "enable shared libs for proto2" FORCE)
add_subdirectory(third_party/protobuf/cmake)

# When using Ninja, compiler output won't be colorized without this.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-fdiagnostics-color=always SUPPORTS_COLOR_ALWAYS)
if(SUPPORTS_COLOR_ALWAYS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
endif()

# Implement ASAN/UBSAN options
if(UPB_ENABLE_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=address")
endif()

if(UPB_ENABLE_UBSAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fsanitize=undefined")
endif()

include_directories(.)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
elseif(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id")
endif()

FIND_PACKAGE(Lua)

if(LUA_FOUND)
  include_directories(${LUA_INCLUDE_DIR})

  add_library(upb_c SHARED
    upb/bindings/lua/upb.c
    upb/bindings/lua/def.c
    upb/bindings/lua/msg.c
  )
  target_link_libraries(upb_c LINK_PRIVATE
    upbpb_pic
    upbdef_pic
    upbhandlers_pic upb_pic )

  add_library(table_c SHARED
    upb/bindings/lua/upb/table.c
  )
  target_link_libraries(table_c LINK_PRIVATE upb_c upb_pic)

  add_library(pb_c SHARED
    upb/bindings/lua/upb/pb.c
  )
  target_link_libraries(pb_c LINK_PRIVATE upb_c upbpb_pic)

  set_target_properties(upb_c
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "lua"
    PREFIX "")
  set_target_properties(table_c pb_c
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "lua/upb"
    PREFIX "")

  add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/tools/upbc
        DEPENDS ${CMAKE_SOURCE_DIR}/tools/upbc
                ${CMAKE_SOURCE_DIR}/tools/upbc.lua
                ${CMAKE_SOURCE_DIR}/tools/dump_cinit.lua
                ${CMAKE_SOURCE_DIR}/tools/make_c_api.lua
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb.lua
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb/table.lua
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb/pb.lua
                upb_c
                table_c
                pb_c
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/tools/upbc
                ${CMAKE_SOURCE_DIR}/tools/upbc.lua
                ${CMAKE_SOURCE_DIR}/tools/dump_cinit.lua
                ${CMAKE_SOURCE_DIR}/tools/make_c_api.lua
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb.lua
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb/table.lua
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb/pb.lua
                ${CMAKE_CURRENT_BINARY_DIR}/tools)
  add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lua/upb.lua
        DEPENDS ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb.lua
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb.lua
                ${CMAKE_CURRENT_BINARY_DIR}/lua)
  add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lua/upb/pb.lua
        DEPENDS ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb/table.lua
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb/pb.lua
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb/table.lua
                ${CMAKE_SOURCE_DIR}/upb/bindings/lua/upb/pb.lua
                ${CMAKE_CURRENT_BINARY_DIR}/lua/upb)
  add_custom_target(
    upbc ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/tools/upbc
            ${CMAKE_CURRENT_BINARY_DIR}/lua/upb.lua
            ${CMAKE_CURRENT_BINARY_DIR}/lua/upb/pb.lua
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/conformance.upb.h
           ${CMAKE_CURRENT_BINARY_DIR}/conformance.upb.c
    DEPENDS upbc ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf/conformance/conformance.proto
    COMMAND protoc --include_imports
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf/conformance/conformance.proto
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf/src/google/protobuf/test_messages_proto3.proto
        -I${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf/conformance
        -I${CMAKE_CURRENT_SOURCE_DIR}/third_party/protobuf/src
        -o${CMAKE_CURRENT_BINARY_DIR}/conformance.pb &&
        tools/upbc ${CMAKE_CURRENT_BINARY_DIR}/conformance.pb
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/descriptor.upb.h
           ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/descriptor.upb.c
    DEPENDS upbc ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/descriptor.proto
    COMMAND protoc
        ${CMAKE_CURRENT_SOURCE_DIR}/google/protobuf/descriptor.proto
        -I${CMAKE_CURRENT_SOURCE_DIR}
        -o${CMAKE_CURRENT_BINARY_DIR}/descriptor.pb &&
        tools/upbc ${CMAKE_CURRENT_BINARY_DIR}/descriptor.pb
  )

  add_custom_target(
    genfiles
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/conformance.upb.h
            ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/descriptor.upb.h
  )
endif()

set(UPB_SRCS
    upb/decode.c
    upb/encode.c
    upb/msg.c
    upb/table.c
    upb/upb.c
)

set (UPBDEF_SRCS
    upb/descriptor/descriptor.upbdefs.c
    upb/descriptor/reader.c
    upb/def.c
    upb/msgfactory.c
    upb/refcounted.c
)

set(UPBHANDLERS_SRCS
    upb/sink.c
    upb/handlers.c
)

set(UPBPB_SRCS
  upb/pb/compile_decoder.c
  upb/pb/decoder.c
  upb/pb/encoder.c
  upb/pb/glue.c
  upb/pb/textprinter.c
  upb/pb/varint.c
)

set(UPBJSON_SRCS
  upb/json/parser.c
  upb/json/printer.c
)

add_library(upb ${UPB_SRCS})
add_library(upbdef ${UPBDEF_SRCS})
add_library(upbhandlers ${UPBHANDLERS_SRCS})
add_library(upbpb ${UPBPB_SRCS})
add_library(upbjson ${UPBJSON_SRCS})

add_library(upb_pic ${UPB_SRCS})
add_library(upbdef_pic ${UPBDEF_SRCS})
add_library(upbhandlers_pic ${UPBHANDLERS_SRCS})
add_library(upbpb_pic ${UPBPB_SRCS})
add_library(upbjson_pic ${UPBJSON_SRCS})

set_property(TARGET upb_pic PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET upbdef_pic PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET upbhandlers_pic PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET upbpb_pic PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET upbjson_pic PROPERTY POSITION_INDEPENDENT_CODE ON)

add_executable(conformance_upb
  tests/conformance_upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/test_messages_proto3.upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/any.upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/duration.upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/field_mask.upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/struct.upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/test_messages_proto3.upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/timestamp.upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/google/protobuf/wrappers.upb.c
  ${CMAKE_CURRENT_BINARY_DIR}/conformance.upb.c
  )
target_link_libraries(conformance_upb LINK_PRIVATE
  upb
)
