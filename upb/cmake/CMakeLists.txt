# This file was generated from BUILD using tools/make_cmakelists.py.

cmake_minimum_required(VERSION 3.10...3.24)

project(upb)
set(CMAKE_C_STANDARD 99)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
if(CMAKE_SOURCE_DIR STREQUAL upb_SOURCE_DIR)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.20)
    set(CMAKE_CXX_STANDARD 23)
  elseif(CMAKE_VERSION VERSION_GREATER_EQUAL 3.12)
    set(CMAKE_CXX_STANDARD 20)
  else()
    set(CMAKE_CXX_STANDARD 17)
  endif()
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
if(CMAKE_CROSSCOMPILING)
  option(UPB_ENABLE_CODEGEN "Build codegen plugins of upb" OFF)
else()
  option(UPB_ENABLE_CODEGEN "Build codegens plugins of upb" ON)
endif()

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

find_package(utf8_range QUIET)
if(NOT TARGET utf8_range AND TARGET utf8_range::utf8_range)
  add_library(utf8_range ALIAS utf8_range::utf8_range)
  if(EXISTS "${utf8_range_DIR}/../../include/utf8_range.h")
    include_directories("${utf8_range_DIR}/../../include/")
  elseif(EXISTS "${utf8_range_DIR}/../../../include/utf8_range.h")
    include_directories("${utf8_range_DIR}/../../../include/")
  endif()
elseif(NOT TARGET utf8_range)
  if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../third_party/utf8_range")
    # utf8_range is already installed
    include_directories("${CMAKE_CURRENT_LIST_DIR}/../../third_party/utf8_range")
  else()
    include(FetchContent)
    FetchContent_Declare(
      utf8_range
      GIT_REPOSITORY "https://github.com/protocolbuffers/utf8_range.git"
      GIT_TAG "d863bc33e15cba6d873c878dcca9e6fe52b2f8cb"
    )
    FetchContent_GetProperties(utf8_range)
    if(NOT utf8_range_POPULATED)
      FetchContent_Populate(utf8_range)
      include_directories(${utf8_range_SOURCE_DIR})
    endif()
  endif()
endif()

if(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup -flat_namespace")
elseif(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id")
endif()

if (MSVC)
  add_compile_options(/wd4146 /wd4703 -D_CRT_SECURE_NO_WARNINGS)
endif()

enable_testing()

if (UPB_ENABLE_CODEGEN)
  find_package(absl CONFIG REQUIRED)
  find_package(protobuf CONFIG REQUIRED)
  if(NOT UPB_HOST_INCLUDE_DIR)
    if(TARGET protobuf::libprotobuf)
      get_target_property(UPB_HOST_INCLUDE_DIR protobuf::libprotobuf INTERFACE_INCLUDE_DIRECTORIES)
    elseif(Protobuf_INCLUDE_DIR)
      set(UPB_HOST_INCLUDE_DIR "${Protobuf_INCLUDE_DIR}")
    else()
      set(UPB_HOST_INCLUDE_DIR "${PROTOBUF_INCLUDE_DIR}")
    endif()
  endif()
endif()


add_library(upb_base_base 
    ../base/status.c
  ../base/descriptor_constants.h
  ../base/status.h
  ../base/status.hpp
  ../base/string_view.h
)
target_include_directories(upb_base_base PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_base_base PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_base_base PUBLIC
  upb_port)

add_library(upb_base ALIAS upb_base_base)
set_target_properties(upb_base_base PROPERTIES EXPORT_NAME upb_base)

add_library(upb_base_internal INTERFACE
    
)
target_include_directories(upb_base_internal INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_base_internal INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_base_internal INTERFACE
  upb_port)

add_library(upb_collections_collections INTERFACE
    
)
target_include_directories(upb_collections_collections INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_collections_collections INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_collections_collections INTERFACE
  upb_message)

add_library(upb_collections ALIAS upb_collections_collections)
set_target_properties(upb_collections_collections PROPERTIES EXPORT_NAME upb_collections)

add_library(upb_hash_hash 
    ../hash/common.c
  ../hash/common.h
  ../hash/int_table.h
  ../hash/str_table.h
)
target_include_directories(upb_hash_hash PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_hash_hash PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_hash_hash PUBLIC
  upb_base
  upb_base_internal
  upb_mem
  upb_port)

add_library(upb_hash ALIAS upb_hash_hash)
set_target_properties(upb_hash_hash PROPERTIES EXPORT_NAME upb_hash)

add_library(upb_io_string INTERFACE
    
)
target_include_directories(upb_io_string INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_io_string INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_io_string INTERFACE
  upb_mem
  upb_port)

add_library(upb_io_tokenizer 
    ../io/tokenizer.c
  ../io/tokenizer.h
)
target_include_directories(upb_io_tokenizer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_io_tokenizer PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_io_tokenizer PUBLIC
  upb_io_string
  upb_io_zero_copy_stream
  upb_base
  upb_lex
  upb_mem
  upb_port)

add_library(upb_io_zero_copy_stream INTERFACE
    
)
target_include_directories(upb_io_zero_copy_stream INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_io_zero_copy_stream INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_io_zero_copy_stream INTERFACE
  upb_base
  upb_mem
  upb_port)

add_library(upb_json_json 
    ../json/decode.c
  ../json/encode.c
  ../json/decode.h
  ../json/encode.h
)
target_include_directories(upb_json_json PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_json_json PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_json_json PUBLIC
  upb_base
  upb_lex
  upb_mem
  upb_message
  upb_message_types
  upb_mini_table
  upb_port
  upb_reflection
  upb_wire)

add_library(upb_json ALIAS upb_json_json)
set_target_properties(upb_json_json PROPERTIES EXPORT_NAME upb_json)

add_library(upb_lex_lex 
    ../lex/atoi.c
  ../lex/round_trip.c
  ../lex/strtod.c
  ../lex/unicode.c
  ../lex/atoi.h
  ../lex/round_trip.h
  ../lex/strtod.h
  ../lex/unicode.h
)
target_include_directories(upb_lex_lex PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_lex_lex PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_lex_lex PUBLIC
  upb_port)

add_library(upb_lex ALIAS upb_lex_lex)
set_target_properties(upb_lex_lex PROPERTIES EXPORT_NAME upb_lex)

add_library(upb_mem_mem INTERFACE
    
)
target_include_directories(upb_mem_mem INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_mem_mem INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_mem_mem INTERFACE
  upb_mem_internal
  upb_port)

add_library(upb_mem ALIAS upb_mem_mem)
set_target_properties(upb_mem_mem PROPERTIES EXPORT_NAME upb_mem)

add_library(upb_mem_internal 
    ../mem/alloc.c
  ../mem/alloc.h
  ../mem/arena.c
  ../mem/arena.h
  ../mem/internal/arena.h
)
target_include_directories(upb_mem_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_mem_internal PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_mem_internal PUBLIC
  upb_port)

add_library(upb_message_accessors 
    ../message/accessors.c
  ../message/internal/accessors.h
  ../message/accessors.h
)
target_include_directories(upb_message_accessors PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_accessors PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_accessors PUBLIC
  upb_message_internal
  upb_message_internal_types
  upb_message_message
  upb_message_tagged_ptr
  upb_base
  upb_eps_copy_input_stream
  upb_mem
  upb_message_types
  upb_mini_table
  upb_mini_table_internal
  upb_port
  upb_wire
  upb_wire_reader)

add_library(upb_message_accessors_internal INTERFACE
    
)
target_include_directories(upb_message_accessors_internal INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_accessors_internal INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_accessors_internal INTERFACE
  upb_message_internal
  upb_message_internal_types
  upb_message_message
  upb_message_tagged_ptr
  upb_base
  upb_mem
  upb_mini_table
  upb_mini_table_internal
  upb_port)

add_library(upb_message_copy 
    ../message/copy.c
  ../message/copy.h
)
target_include_directories(upb_message_copy PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_copy PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_copy PUBLIC
  upb_message_accessors
  upb_message_accessors_internal
  upb_message_internal
  upb_message_message
  upb_message_tagged_ptr
  upb_message_types
  upb_base
  upb_mem
  upb_mini_table
  upb_mini_table_internal
  upb_port)

add_library(upb_message_internal 
    ../message/array.c
  ../message/array.h
  ../message/internal/extension.c
  ../message/internal/message.c
  ../message/map.c
  ../message/map.h
  ../message/map_sorter.c
  ../message/message.c
  ../message/message.h
  ../message/internal/array.h
  ../message/internal/extension.h
  ../message/internal/map.h
  ../message/internal/map_entry.h
  ../message/internal/map_sorter.h
  ../message/internal/message.h
  ../message/map_gencode_util.h
)
target_include_directories(upb_message_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_internal PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_internal PUBLIC
  upb_message_internal_types
  upb_message_types
  upb_message_value
  upb_base
  upb_base_internal
  upb_hash
  upb_mem
  upb_mini_table
  upb_mini_table_internal
  upb_port)

add_library(upb_message_internal_types INTERFACE
    
)
target_include_directories(upb_message_internal_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_internal_types INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_internal_types INTERFACE
  )

add_library(upb_message_message 
    ../message/compat.c
  ../message/array.h
  ../message/compat.h
  ../message/map.h
  ../message/message.h
)
target_include_directories(upb_message_message PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_message PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_message PUBLIC
  upb_message_internal
  upb_message_types
  upb_message_value
  upb_base
  upb_mem
  upb_mini_table
  upb_port)

add_library(upb_message ALIAS upb_message_message)
set_target_properties(upb_message_message PROPERTIES EXPORT_NAME upb_message)

add_library(upb_message_promote 
    ../message/promote.c
  ../message/promote.h
)
target_include_directories(upb_message_promote PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_promote PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_promote PUBLIC
  upb_message_accessors
  upb_message_accessors_internal
  upb_message_internal
  upb_message_message
  upb_message_tagged_ptr
  upb_message_types
  upb_base
  upb_eps_copy_input_stream
  upb_mem
  upb_mini_table
  upb_mini_table_internal
  upb_port
  upb_wire
  upb_wire_internal
  upb_wire_reader)

add_library(upb_message_split64 INTERFACE
    
)
target_include_directories(upb_message_split64 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_split64 INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_split64 INTERFACE
  upb_message_accessors
  upb_message_message
  upb_port)

add_library(upb_message_tagged_ptr INTERFACE
    
)
target_include_directories(upb_message_tagged_ptr INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_tagged_ptr INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_tagged_ptr INTERFACE
  upb_message_types
  upb_port)

add_library(upb_message_types INTERFACE
    
)
target_include_directories(upb_message_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_types INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_types INTERFACE
  )

add_library(upb_message_value INTERFACE
    
)
target_include_directories(upb_message_value INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_message_value INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_message_value INTERFACE
  upb_message_tagged_ptr
  upb_message_types
  upb_base)

add_library(upb_mini_descriptor_mini_descriptor 
    ../mini_descriptor/build_enum.c
  ../mini_descriptor/decode.c
  ../mini_descriptor/link.c
  ../mini_descriptor/build_enum.h
  ../mini_descriptor/decode.h
  ../mini_descriptor/link.h
)
target_include_directories(upb_mini_descriptor_mini_descriptor PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_mini_descriptor_mini_descriptor PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_mini_descriptor_mini_descriptor PUBLIC
  upb_mini_descriptor_internal
  upb_base
  upb_base_internal
  upb_mem
  upb_mini_table
  upb_mini_table_internal
  upb_port)

add_library(upb_mini_descriptor ALIAS upb_mini_descriptor_mini_descriptor)
set_target_properties(upb_mini_descriptor_mini_descriptor PROPERTIES EXPORT_NAME upb_mini_descriptor)

add_library(upb_mini_descriptor_internal 
    ../mini_descriptor/internal/base92.c
  ../mini_descriptor/internal/encode.c
  ../mini_descriptor/internal/base92.h
  ../mini_descriptor/internal/decoder.h
  ../mini_descriptor/internal/encode.h
  ../mini_descriptor/internal/encode.hpp
  ../mini_descriptor/internal/modifiers.h
  ../mini_descriptor/internal/wire_constants.h
)
target_include_directories(upb_mini_descriptor_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_mini_descriptor_internal PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_mini_descriptor_internal PUBLIC
  upb_base
  upb_base_internal
  upb_port)

add_library(upb_mini_table_compat 
    ../mini_table/compat.c
  ../mini_table/compat.h
)
target_include_directories(upb_mini_table_compat PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_mini_table_compat PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_mini_table_compat PUBLIC
  upb_mini_table_mini_table
  upb_base
  upb_hash
  upb_mem
  upb_port)

add_library(upb_mini_table_mini_table 
    ../mini_table/extension_registry.c
  ../mini_table/message.c
  ../mini_table/enum.h
  ../mini_table/extension.h
  ../mini_table/extension_registry.h
  ../mini_table/field.h
  ../mini_table/file.h
  ../mini_table/message.h
  ../mini_table/sub.h
)
target_include_directories(upb_mini_table_mini_table PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_mini_table_mini_table PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_mini_table_mini_table PUBLIC
  upb_mini_table_internal
  upb_base
  upb_hash
  upb_mem
  upb_port)

add_library(upb_mini_table ALIAS upb_mini_table_mini_table)
set_target_properties(upb_mini_table_mini_table PROPERTIES EXPORT_NAME upb_mini_table)

add_library(upb_mini_table_internal 
    ../mini_table/internal/message.c
  ../mini_table/internal/enum.h
  ../mini_table/internal/extension.h
  ../mini_table/internal/field.h
  ../mini_table/internal/file.h
  ../mini_table/internal/message.h
  ../mini_table/internal/size_log2.h
  ../mini_table/internal/sub.h
)
target_include_directories(upb_mini_table_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_mini_table_internal PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_mini_table_internal PUBLIC
  upb_base
  upb_message_types
  upb_port)

add_library(upb_port_port INTERFACE
    
)
target_include_directories(upb_port_port INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_port_port INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()

add_library(upb_port ALIAS upb_port_port)
set_target_properties(upb_port_port PROPERTIES EXPORT_NAME upb_port)
if (UPB_ENABLE_CODEGEN)

add_library(upb_reflection_descriptor_upb_proto_stage0 
    ../reflection/stage0/google/protobuf/descriptor.upb.h
  ../reflection/stage0/google/protobuf/descriptor.upb.c
)
target_include_directories(upb_reflection_descriptor_upb_proto_stage0 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_descriptor_upb_proto_stage0 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_reflection_descriptor_upb_proto_stage0
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../reflection/stage0>")
target_link_libraries(upb_reflection_descriptor_upb_proto_stage0 PUBLIC
  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me
  upb_mini_table)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto")
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.h
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.c
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.h
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.c
  DEPENDS
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/descriptor.proto
  COMMAND
    "${PROTOC_PROGRAM}"
    "-I${UPB_HOST_INCLUDE_DIR}"
    "--plugin=protoc-gen-upb=\$<TARGET_FILE:upb_generator_protoc-gen-upb_stage0>"
    "--plugin=protoc-gen-upb_minitable=\$<TARGET_FILE:upb_generator_protoc-gen-upb_minitable_stage0>"
    "--upb_out=${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto"
    "--upb_minitable_out=${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto"
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/descriptor.proto
)

add_library(upb_reflection_descriptor_upb_proto_minitable_stage1 
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.h
  ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.c
)
target_include_directories(upb_reflection_descriptor_upb_proto_minitable_stage1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_descriptor_upb_proto_minitable_stage1 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_reflection_descriptor_upb_proto_minitable_stage1
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto>")
target_link_libraries(upb_reflection_descriptor_upb_proto_minitable_stage1 PUBLIC
  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me
)

add_library(upb_reflection_descriptor_upb_proto_stage1 
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.h
  ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.c
)
target_include_directories(upb_reflection_descriptor_upb_proto_stage1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_descriptor_upb_proto_stage1 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_reflection_descriptor_upb_proto_stage1
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_reflection_descriptor_upb_proto>")
target_link_libraries(upb_reflection_descriptor_upb_proto_stage1 PUBLIC
  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me
  upb_reflection_descriptor_upb_proto_minitable_stage1
)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto")
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.c
  DEPENDS
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/descriptor.proto
  COMMAND
    "${PROTOC_PROGRAM}"
    "-I${UPB_HOST_INCLUDE_DIR}"
    "--plugin=protoc-gen-upb=\$<TARGET_FILE:upb_generator_protoc-gen-upb_stage1>"
    "--plugin=protoc-gen-upb_minitable=\$<TARGET_FILE:upb_generator_protoc-gen-upb_minitable_stage1>"
    "--upb_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto"
    "--upb_minitable_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto"
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/descriptor.proto
)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upbdefs.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upbdefs.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor_pb.lua
  DEPENDS
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/descriptor.proto
  COMMAND
    "${PROTOC_PROGRAM}"
    "-I${UPB_HOST_INCLUDE_DIR}"
    "--plugin=protoc-gen-upbdefs=\$<TARGET_FILE:upb_generator_protoc-gen-upbdefs>"
    "--plugin=protoc-gen-lua=\$<TARGET_FILE:lua_protoc-gen-lua>"
    "--upbdefs_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto"
    "--lua_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto"
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/descriptor.proto
)

add_library(upb_reflection_descriptor_upb_proto_minitable 
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.h
  ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.c
)
target_include_directories(upb_reflection_descriptor_upb_proto_minitable PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_descriptor_upb_proto_minitable PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_reflection_descriptor_upb_proto_minitable
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto>")
target_link_libraries(upb_reflection_descriptor_upb_proto_minitable PUBLIC
  upb_upb
)

add_library(upb_reflection_descriptor_upb_proto 
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.h
  ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.c
)
target_include_directories(upb_reflection_descriptor_upb_proto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_descriptor_upb_proto PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_reflection_descriptor_upb_proto
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto>")
target_link_libraries(upb_reflection_descriptor_upb_proto PUBLIC
  upb_upb
  upb_reflection_descriptor_upb_proto_minitable
)

add_library(upb_reflection_descriptor_upb_proto_defs 
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upbdefs.h
  ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upbdefs.c
)
target_include_directories(upb_reflection_descriptor_upb_proto_defs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_descriptor_upb_proto_defs PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_reflection_descriptor_upb_proto_defs
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto>")
target_link_libraries(upb_reflection_descriptor_upb_proto_defs PUBLIC
  upb_reflection_descriptor_upb_proto
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upb_minitable.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upbdefs.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor.upbdefs.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_reflection_descriptor_upb_proto/google/protobuf/descriptor_pb.lua
  DESTINATION "include/google/protobuf"
)
endif()
if (UPB_ENABLE_CODEGEN)

add_library(upb_reflection_reflection_stage0 INTERFACE
    
)
target_include_directories(upb_reflection_reflection_stage0 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_reflection_stage0 INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_reflection_reflection_stage0 INTERFACE
  upb_base
  upb_mem
  upb_message
  upb_message_value
  upb_mini_descriptor
  upb_mini_table
  upb_port)
target_link_libraries(upb_reflection_reflection_stage0 INTERFACE
  upb_reflection_internal_stage0)

add_library(upb_reflection_reflection_stage1 INTERFACE
    
)
target_include_directories(upb_reflection_reflection_stage1 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_reflection_stage1 INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_reflection_reflection_stage1 INTERFACE
  upb_base
  upb_mem
  upb_message
  upb_message_value
  upb_mini_descriptor
  upb_mini_table
  upb_port)
target_link_libraries(upb_reflection_reflection_stage1 INTERFACE
  upb_reflection_internal_stage1)

add_library(upb_reflection_reflection INTERFACE
    
)
target_include_directories(upb_reflection_reflection INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_reflection INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_reflection_reflection INTERFACE
  upb_base
  upb_mem
  upb_message
  upb_message_value
  upb_mini_descriptor
  upb_mini_table
  upb_port)
target_link_libraries(upb_reflection_reflection INTERFACE
  upb_reflection_internal)

add_library(upb_reflection ALIAS upb_reflection_reflection)
set_target_properties(upb_reflection_reflection PROPERTIES EXPORT_NAME upb_reflection)
endif()
if (UPB_ENABLE_CODEGEN)

add_library(upb_reflection_internal_stage0 
    ../reflection/def_pool.c
  ../reflection/def_type.c
  ../reflection/desc_state.c
  ../reflection/enum_def.c
  ../reflection/enum_reserved_range.c
  ../reflection/enum_value_def.c
  ../reflection/extension_range.c
  ../reflection/field_def.c
  ../reflection/file_def.c
  ../reflection/internal/def_builder.c
  ../reflection/internal/def_builder.h
  ../reflection/internal/strdup2.c
  ../reflection/internal/strdup2.h
  ../reflection/message.c
  ../reflection/message_def.c
  ../reflection/message_reserved_range.c
  ../reflection/method_def.c
  ../reflection/oneof_def.c
  ../reflection/service_def.c
  ../reflection/common.h
  ../reflection/def.h
  ../reflection/def.hpp
  ../reflection/def_pool.h
  ../reflection/def_type.h
  ../reflection/enum_def.h
  ../reflection/enum_reserved_range.h
  ../reflection/enum_value_def.h
  ../reflection/extension_range.h
  ../reflection/field_def.h
  ../reflection/file_def.h
  ../reflection/internal/def_pool.h
  ../reflection/internal/desc_state.h
  ../reflection/internal/enum_def.h
  ../reflection/internal/enum_reserved_range.h
  ../reflection/internal/enum_value_def.h
  ../reflection/internal/extension_range.h
  ../reflection/internal/field_def.h
  ../reflection/internal/file_def.h
  ../reflection/internal/message_def.h
  ../reflection/internal/message_reserved_range.h
  ../reflection/internal/method_def.h
  ../reflection/internal/oneof_def.h
  ../reflection/internal/service_def.h
  ../reflection/internal/upb_edition_defaults.h
  ../reflection/message.h
  ../reflection/message.hpp
  ../reflection/message_def.h
  ../reflection/message_reserved_range.h
  ../reflection/method_def.h
  ../reflection/oneof_def.h
  ../reflection/service_def.h
)
target_include_directories(upb_reflection_internal_stage0 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_internal_stage0 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_reflection_internal_stage0 PUBLIC
  upb_base
  upb_hash
  upb_mem
  upb_message
  upb_message_accessors
  upb_message_accessors_internal
  upb_message_copy
  upb_message_internal
  upb_message_value
  upb_mini_descriptor
  upb_mini_descriptor_internal
  upb_mini_table
  upb_mini_table_internal
  upb_port
  upb_base_internal)
target_link_libraries(upb_reflection_internal_stage0 PUBLIC
  upb_reflection_descriptor_upb_proto_stage0)

add_library(upb_reflection_internal_stage1 
    ../reflection/def_pool.c
  ../reflection/def_type.c
  ../reflection/desc_state.c
  ../reflection/enum_def.c
  ../reflection/enum_reserved_range.c
  ../reflection/enum_value_def.c
  ../reflection/extension_range.c
  ../reflection/field_def.c
  ../reflection/file_def.c
  ../reflection/internal/def_builder.c
  ../reflection/internal/def_builder.h
  ../reflection/internal/strdup2.c
  ../reflection/internal/strdup2.h
  ../reflection/message.c
  ../reflection/message_def.c
  ../reflection/message_reserved_range.c
  ../reflection/method_def.c
  ../reflection/oneof_def.c
  ../reflection/service_def.c
  ../reflection/common.h
  ../reflection/def.h
  ../reflection/def.hpp
  ../reflection/def_pool.h
  ../reflection/def_type.h
  ../reflection/enum_def.h
  ../reflection/enum_reserved_range.h
  ../reflection/enum_value_def.h
  ../reflection/extension_range.h
  ../reflection/field_def.h
  ../reflection/file_def.h
  ../reflection/internal/def_pool.h
  ../reflection/internal/desc_state.h
  ../reflection/internal/enum_def.h
  ../reflection/internal/enum_reserved_range.h
  ../reflection/internal/enum_value_def.h
  ../reflection/internal/extension_range.h
  ../reflection/internal/field_def.h
  ../reflection/internal/file_def.h
  ../reflection/internal/message_def.h
  ../reflection/internal/message_reserved_range.h
  ../reflection/internal/method_def.h
  ../reflection/internal/oneof_def.h
  ../reflection/internal/service_def.h
  ../reflection/internal/upb_edition_defaults.h
  ../reflection/message.h
  ../reflection/message.hpp
  ../reflection/message_def.h
  ../reflection/message_reserved_range.h
  ../reflection/method_def.h
  ../reflection/oneof_def.h
  ../reflection/service_def.h
)
target_include_directories(upb_reflection_internal_stage1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_internal_stage1 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_reflection_internal_stage1 PUBLIC
  upb_base
  upb_hash
  upb_mem
  upb_message
  upb_message_accessors
  upb_message_accessors_internal
  upb_message_copy
  upb_message_internal
  upb_message_value
  upb_mini_descriptor
  upb_mini_descriptor_internal
  upb_mini_table
  upb_mini_table_internal
  upb_port
  upb_base_internal)
target_link_libraries(upb_reflection_internal_stage1 PUBLIC
  upb_reflection_descriptor_upb_proto_stage1)

add_library(upb_reflection_internal 
    ../reflection/def_pool.c
  ../reflection/def_type.c
  ../reflection/desc_state.c
  ../reflection/enum_def.c
  ../reflection/enum_reserved_range.c
  ../reflection/enum_value_def.c
  ../reflection/extension_range.c
  ../reflection/field_def.c
  ../reflection/file_def.c
  ../reflection/internal/def_builder.c
  ../reflection/internal/def_builder.h
  ../reflection/internal/strdup2.c
  ../reflection/internal/strdup2.h
  ../reflection/message.c
  ../reflection/message_def.c
  ../reflection/message_reserved_range.c
  ../reflection/method_def.c
  ../reflection/oneof_def.c
  ../reflection/service_def.c
  ../reflection/common.h
  ../reflection/def.h
  ../reflection/def.hpp
  ../reflection/def_pool.h
  ../reflection/def_type.h
  ../reflection/enum_def.h
  ../reflection/enum_reserved_range.h
  ../reflection/enum_value_def.h
  ../reflection/extension_range.h
  ../reflection/field_def.h
  ../reflection/file_def.h
  ../reflection/internal/def_pool.h
  ../reflection/internal/desc_state.h
  ../reflection/internal/enum_def.h
  ../reflection/internal/enum_reserved_range.h
  ../reflection/internal/enum_value_def.h
  ../reflection/internal/extension_range.h
  ../reflection/internal/field_def.h
  ../reflection/internal/file_def.h
  ../reflection/internal/message_def.h
  ../reflection/internal/message_reserved_range.h
  ../reflection/internal/method_def.h
  ../reflection/internal/oneof_def.h
  ../reflection/internal/service_def.h
  ../reflection/internal/upb_edition_defaults.h
  ../reflection/message.h
  ../reflection/message.hpp
  ../reflection/message_def.h
  ../reflection/message_reserved_range.h
  ../reflection/method_def.h
  ../reflection/oneof_def.h
  ../reflection/service_def.h
)
target_include_directories(upb_reflection_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_internal PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_reflection_internal PUBLIC
  upb_base
  upb_hash
  upb_mem
  upb_message
  upb_message_accessors
  upb_message_accessors_internal
  upb_message_copy
  upb_message_internal
  upb_message_value
  upb_mini_descriptor
  upb_mini_descriptor_internal
  upb_mini_table
  upb_mini_table_internal
  upb_port
  upb_base_internal)
target_link_libraries(upb_reflection_internal PUBLIC
  upb_reflection_descriptor_upb_proto)
endif()

add_library(upb_reflection_generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(upb_reflection_generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_reflection_generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_reflection_generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  upb_mem
  upb_mini_descriptor
  upb_reflection_internal)

add_library(upb_text_text 
    ../text/encode.c
  ../text/encode.h
)
target_include_directories(upb_text_text PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_text_text PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_text_text PUBLIC
  upb_eps_copy_input_stream
  upb_lex
  upb_message
  upb_message_internal
  upb_port
  upb_reflection
  upb_wire
  upb_wire_reader
  upb_wire_types)

add_library(upb_text ALIAS upb_text_text)
set_target_properties(upb_text_text PROPERTIES EXPORT_NAME upb_text)

add_library(upb_util_def_to_proto 
    ../util/def_to_proto.c
  ../util/def_to_proto.h
)
target_include_directories(upb_util_def_to_proto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_util_def_to_proto PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_util_def_to_proto PUBLIC
  upb_port
  upb_reflection
  upb_reflection_internal)

add_library(upb_util_required_fields 
    ../util/required_fields.c
  ../util/required_fields.h
)
target_include_directories(upb_util_required_fields PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_util_required_fields PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_util_required_fields PUBLIC
  upb_base
  upb_mem
  upb_message
  upb_port
  upb_reflection)

add_library(upb_util_compare 
    ../util/compare.c
  ../util/compare.h
)
target_include_directories(upb_util_compare PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_util_compare PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_util_compare PUBLIC
  upb_base
  upb_eps_copy_input_stream
  upb_mem
  upb_port
  upb_wire_reader
  upb_wire_types)

add_library(upb_wire_wire INTERFACE
    
)
target_include_directories(upb_wire_wire INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_wire_wire INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_wire_wire INTERFACE
  upb_wire_internal
  upb_wire_types
  upb_mem
  upb_message
  upb_mini_table
  upb_port)

add_library(upb_wire ALIAS upb_wire_wire)
set_target_properties(upb_wire_wire PROPERTIES EXPORT_NAME upb_wire)

add_library(upb_wire_internal 
    ../wire/decode.c
  ../wire/decode.h
  ../wire/decode_fast.c
  ../wire/encode.c
  ../wire/encode.h
  ../wire/decode_fast.h
  ../wire/internal/constants.h
  ../wire/internal/decode.h
  ../wire/internal/swap.h
)
target_include_directories(upb_wire_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_wire_internal PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_wire_internal PUBLIC
  upb_wire_eps_copy_input_stream
  upb_wire_reader
  upb_wire_types
  upb_base
  upb_hash
  upb_mem
  upb_mem_internal
  upb_message
  upb_message_accessors_internal
  upb_message_internal
  upb_message_internal_types
  upb_message_tagged_ptr
  upb_mini_table
  upb_mini_table_internal
  upb_port
  utf8_range)

add_library(upb_wire_reader 
    ../wire/internal/swap.h
  ../wire/reader.c
  ../wire/reader.h
)
target_include_directories(upb_wire_reader PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_wire_reader PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_wire_reader PUBLIC
  upb_wire_eps_copy_input_stream
  upb_wire_types
  upb_port)

add_library(upb_wire_types INTERFACE
    
)
target_include_directories(upb_wire_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_wire_types INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()

add_library(upb_wire_eps_copy_input_stream 
    ../wire/eps_copy_input_stream.c
  ../wire/eps_copy_input_stream.h
)
target_include_directories(upb_wire_eps_copy_input_stream PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_wire_eps_copy_input_stream PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_wire_eps_copy_input_stream PUBLIC
  upb_mem
  upb_port)

add_library(upb_upb INTERFACE
    
)
target_include_directories(upb_upb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_upb INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_upb INTERFACE
  upb_base
  upb_mem)
set_target_properties(upb_upb PROPERTIES EXPORT_NAME upb)

add_library(upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  upb_base
  upb_mem
  upb_message
  upb_message_accessors
  upb_message_accessors_internal
  upb_message_internal
  upb_mini_descriptor
  upb_mini_table
  upb_wire
  upb_wire_internal)
set_target_properties(upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me PROPERTIES EXPORT_NAME generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me)

add_library(upb_generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(upb_generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(upb_generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me PROPERTIES EXPORT_NAME generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me)

add_library(upb_generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me ALIAS upb_reflection_generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me)

add_library(upb_eps_copy_input_stream ALIAS upb_wire_eps_copy_input_stream)
if (UPB_ENABLE_CODEGEN)

add_library(upb_generator_plugin_upb_proto_stage0 
    ../../upb_generator/stage0/google/protobuf/compiler/plugin.upb.h
  ../../upb_generator/stage0/google/protobuf/compiler/plugin.upb.c
)
target_include_directories(upb_generator_plugin_upb_proto_stage0 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin_upb_proto_stage0 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_generator_plugin_upb_proto_stage0
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../upb_generator/stage0>")
target_link_libraries(upb_generator_plugin_upb_proto_stage0 PUBLIC
  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me
  upb_mini_table)
target_link_libraries(upb_generator_plugin_upb_proto_stage0 PUBLIC
  upb_reflection_descriptor_upb_proto_stage0)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto")
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.h
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.c
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.h
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.c
  DEPENDS
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/compiler/plugin.proto
  COMMAND
    "${PROTOC_PROGRAM}"
    "-I${UPB_HOST_INCLUDE_DIR}"
    "--plugin=protoc-gen-upb=\$<TARGET_FILE:upb_generator_protoc-gen-upb_stage0>"
    "--plugin=protoc-gen-upb_minitable=\$<TARGET_FILE:upb_generator_protoc-gen-upb_minitable_stage0>"
    "--upb_out=${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto"
    "--upb_minitable_out=${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto"
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/compiler/plugin.proto
)

add_library(upb_generator_plugin_upb_proto_minitable_stage1 
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.h
  ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.c
)
target_include_directories(upb_generator_plugin_upb_proto_minitable_stage1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin_upb_proto_minitable_stage1 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_generator_plugin_upb_proto_minitable_stage1
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto>")
target_link_libraries(upb_generator_plugin_upb_proto_minitable_stage1 PUBLIC
  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me
)
target_link_libraries(upb_generator_plugin_upb_proto_minitable_stage1 PUBLIC
  upb_reflection_descriptor_upb_proto_stage1)

add_library(upb_generator_plugin_upb_proto_stage1 
    ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.h
  ${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.c
)
target_include_directories(upb_generator_plugin_upb_proto_stage1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin_upb_proto_stage1 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_generator_plugin_upb_proto_stage1
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/_stage1/upb_generator_plugin_upb_proto>")
target_link_libraries(upb_generator_plugin_upb_proto_stage1 PUBLIC
  upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me
  upb_generator_plugin_upb_proto_minitable_stage1
)
target_link_libraries(upb_generator_plugin_upb_proto_stage1 PUBLIC
  upb_reflection_descriptor_upb_proto_stage1)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto")
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.c
  DEPENDS
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/compiler/plugin.proto
  COMMAND
    "${PROTOC_PROGRAM}"
    "-I${UPB_HOST_INCLUDE_DIR}"
    "--plugin=protoc-gen-upb=\$<TARGET_FILE:upb_generator_protoc-gen-upb_stage1>"
    "--plugin=protoc-gen-upb_minitable=\$<TARGET_FILE:upb_generator_protoc-gen-upb_minitable_stage1>"
    "--upb_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto"
    "--upb_minitable_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto"
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/compiler/plugin.proto
)
add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upbdefs.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upbdefs.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin_pb.lua
  DEPENDS
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/compiler/plugin.proto
  COMMAND
    "${PROTOC_PROGRAM}"
    "-I${UPB_HOST_INCLUDE_DIR}"
    "--plugin=protoc-gen-upbdefs=\$<TARGET_FILE:upb_generator_protoc-gen-upbdefs>"
    "--plugin=protoc-gen-lua=\$<TARGET_FILE:lua_protoc-gen-lua>"
    "--upbdefs_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto"
    "--lua_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto"
    ${UPB_HOST_INCLUDE_DIR}/google/protobuf/compiler/plugin.proto
)

add_library(upb_generator_plugin_upb_proto_minitable 
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.h
  ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.c
)
target_include_directories(upb_generator_plugin_upb_proto_minitable PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin_upb_proto_minitable PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_generator_plugin_upb_proto_minitable
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto>")
target_link_libraries(upb_generator_plugin_upb_proto_minitable PUBLIC
  upb_upb
)
target_link_libraries(upb_generator_plugin_upb_proto_minitable PUBLIC
  upb_reflection_descriptor_upb_proto)

add_library(upb_generator_plugin_upb_proto 
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.h
  ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.c
)
target_include_directories(upb_generator_plugin_upb_proto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin_upb_proto PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_generator_plugin_upb_proto
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto>")
target_link_libraries(upb_generator_plugin_upb_proto PUBLIC
  upb_upb
  upb_generator_plugin_upb_proto_minitable
)
target_link_libraries(upb_generator_plugin_upb_proto PUBLIC
  upb_reflection_descriptor_upb_proto)

add_library(upb_generator_plugin_upb_proto_defs 
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upbdefs.h
  ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upbdefs.c
)
target_include_directories(upb_generator_plugin_upb_proto_defs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin_upb_proto_defs PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_include_directories(upb_generator_plugin_upb_proto_defs
  BEFORE PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto>")
target_link_libraries(upb_generator_plugin_upb_proto_defs PUBLIC
  upb_generator_plugin_upb_proto
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upb_minitable.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upbdefs.h
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin.upbdefs.c
    ${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_generator_plugin_upb_proto/google/protobuf/compiler/plugin_pb.lua
  DESTINATION "include/google/protobuf/compiler"
)
endif()
if (UPB_ENABLE_CODEGEN)

add_library(upb_generator_common_stage0 
    ../../upb_generator/common.cc
  ../../upb_generator/common.h
)
target_include_directories(upb_generator_common_stage0 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_common_stage0 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_common_stage0 PUBLIC
  upb_generator_mangle
  upb_mini_table
  upb_mini_table_internal
  upb_port
  absl::strings)
target_link_libraries(upb_generator_common_stage0 PUBLIC
  upb_reflection_reflection_stage0)

add_library(upb_generator_common_stage1 
    ../../upb_generator/common.cc
  ../../upb_generator/common.h
)
target_include_directories(upb_generator_common_stage1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_common_stage1 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_common_stage1 PUBLIC
  upb_generator_mangle
  upb_mini_table
  upb_mini_table_internal
  upb_port
  absl::strings)
target_link_libraries(upb_generator_common_stage1 PUBLIC
  upb_reflection_reflection_stage1)

add_library(upb_generator_common 
    ../../upb_generator/common.cc
  ../../upb_generator/common.h
)
target_include_directories(upb_generator_common PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_common PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_common PUBLIC
  upb_generator_mangle
  upb_mini_table
  upb_mini_table_internal
  upb_port
  absl::strings)
target_link_libraries(upb_generator_common PUBLIC
  upb_reflection_reflection)
endif()
if (UPB_ENABLE_CODEGEN)

add_library(upb_generator_file_layout_stage0 
    ../../upb_generator/file_layout.cc
  ../../upb_generator/file_layout.h
)
target_include_directories(upb_generator_file_layout_stage0 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_file_layout_stage0 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_file_layout_stage0 PUBLIC
  upb_base
  upb_mini_descriptor
  upb_mini_table
  upb_mini_table_internal
  upb_port
  absl::flat_hash_map
  absl::strings)
target_link_libraries(upb_generator_file_layout_stage0 PUBLIC
  upb_generator_common_stage0
  upb_reflection_reflection_stage0
  upb_reflection_descriptor_upb_proto_stage0)

add_library(upb_generator_file_layout_stage1 
    ../../upb_generator/file_layout.cc
  ../../upb_generator/file_layout.h
)
target_include_directories(upb_generator_file_layout_stage1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_file_layout_stage1 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_file_layout_stage1 PUBLIC
  upb_base
  upb_mini_descriptor
  upb_mini_table
  upb_mini_table_internal
  upb_port
  absl::flat_hash_map
  absl::strings)
target_link_libraries(upb_generator_file_layout_stage1 PUBLIC
  upb_generator_common_stage1
  upb_reflection_reflection_stage1
  upb_reflection_descriptor_upb_proto_stage1)

add_library(upb_generator_file_layout 
    ../../upb_generator/file_layout.cc
  ../../upb_generator/file_layout.h
)
target_include_directories(upb_generator_file_layout PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_file_layout PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_file_layout PUBLIC
  upb_base
  upb_mini_descriptor
  upb_mini_table
  upb_mini_table_internal
  upb_port
  absl::flat_hash_map
  absl::strings)
target_link_libraries(upb_generator_file_layout PUBLIC
  upb_generator_common
  upb_reflection_reflection
  upb_reflection_descriptor_upb_proto)
endif()

add_library(upb_generator_keywords 
    ../../upb_generator/keywords.cc
  ../../upb_generator/keywords.h
)
target_include_directories(upb_generator_keywords PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_keywords PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
if (UPB_ENABLE_CODEGEN)

add_library(upb_generator_plugin_stage0 INTERFACE
    
)
target_include_directories(upb_generator_plugin_stage0 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin_stage0 INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_plugin_stage0 INTERFACE
  upb_port
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_plugin_stage0 INTERFACE
  upb_generator_plugin_upb_proto_stage0
  upb_reflection_descriptor_upb_proto_stage0
  upb_reflection_reflection_stage0)

add_library(upb_generator_plugin_stage1 INTERFACE
    
)
target_include_directories(upb_generator_plugin_stage1 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin_stage1 INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_plugin_stage1 INTERFACE
  upb_port
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_plugin_stage1 INTERFACE
  upb_generator_plugin_upb_proto_stage1
  upb_reflection_descriptor_upb_proto_stage1
  upb_reflection_reflection_stage1)

add_library(upb_generator_plugin INTERFACE
    
)
target_include_directories(upb_generator_plugin INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_plugin INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_plugin INTERFACE
  upb_port
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_plugin INTERFACE
  upb_generator_plugin_upb_proto
  upb_reflection_descriptor_upb_proto
  upb_reflection_reflection)
endif()
if (UPB_ENABLE_CODEGEN)

add_library(upb_generator_names_stage0 
    ../../upb_generator/names.cc
  ../../upb_generator/names.h
)
target_include_directories(upb_generator_names_stage0 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_names_stage0 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_names_stage0 PUBLIC
  protobuf::libprotobuf
  protobuf::libprotoc
  absl::core_headers
  absl::flat_hash_map
  absl::strings)
target_link_libraries(upb_generator_names_stage0 PUBLIC
  upb_reflection_reflection_stage0)

add_library(upb_generator_names_stage1 
    ../../upb_generator/names.cc
  ../../upb_generator/names.h
)
target_include_directories(upb_generator_names_stage1 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_names_stage1 PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_names_stage1 PUBLIC
  protobuf::libprotobuf
  protobuf::libprotoc
  absl::core_headers
  absl::flat_hash_map
  absl::strings)
target_link_libraries(upb_generator_names_stage1 PUBLIC
  upb_reflection_reflection_stage1)

add_library(upb_generator_names 
    ../../upb_generator/names.cc
  ../../upb_generator/names.h
)
target_include_directories(upb_generator_names PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_names PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_names PUBLIC
  protobuf::libprotobuf
  protobuf::libprotoc
  absl::core_headers
  absl::flat_hash_map
  absl::strings)
target_link_libraries(upb_generator_names PUBLIC
  upb_reflection_reflection)
endif()

add_library(upb_generator_mangle 
    ../../upb_generator/mangle.cc
  ../../upb_generator/mangle.h
)
target_include_directories(upb_generator_mangle PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_mangle PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(upb_generator_mangle PUBLIC
  absl::strings)
if (UPB_ENABLE_CODEGEN)

add_executable(upb_generator_protoc-gen-upb_stage0
    ../../upb_generator/protoc-gen-upb.cc
)
target_include_directories(upb_generator_protoc-gen-upb_stage0 PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_protoc-gen-upb_stage0 PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(upb_generator_protoc-gen-upb_stage0 PROPERTIES OUTPUT_NAME protoc-gen-upb_stage0 EXPORT_NAME protoc-gen-upb_stage0)
target_link_libraries(upb_generator_protoc-gen-upb_stage0 PRIVATE
  upb_base
  upb_mem
  upb_mini_table_internal
  upb_port
  upb_wire_types
  absl::flat_hash_map
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_protoc-gen-upb_stage0 PRIVATE
  upb_generator_common_stage0
  upb_generator_file_layout_stage0
  upb_generator_names_stage0
  upb_generator_plugin_stage0
  upb_generator_plugin_upb_proto_stage0
  upb_reflection_descriptor_upb_proto_stage0
  upb_reflection_reflection_stage0)

add_executable(upb_generator_protoc-gen-upb_stage1
    ../../upb_generator/protoc-gen-upb.cc
)
target_include_directories(upb_generator_protoc-gen-upb_stage1 PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_protoc-gen-upb_stage1 PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(upb_generator_protoc-gen-upb_stage1 PROPERTIES OUTPUT_NAME protoc-gen-upb_stage1 EXPORT_NAME protoc-gen-upb_stage1)
target_link_libraries(upb_generator_protoc-gen-upb_stage1 PRIVATE
  upb_base
  upb_mem
  upb_mini_table_internal
  upb_port
  upb_wire_types
  absl::flat_hash_map
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_protoc-gen-upb_stage1 PRIVATE
  upb_generator_common_stage1
  upb_generator_file_layout_stage1
  upb_generator_names_stage1
  upb_generator_plugin_stage1
  upb_generator_plugin_upb_proto_stage1
  upb_reflection_descriptor_upb_proto_stage1
  upb_reflection_reflection_stage1)

add_executable(upb_generator_protoc-gen-upb
    ../../upb_generator/protoc-gen-upb.cc
)
target_include_directories(upb_generator_protoc-gen-upb PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_protoc-gen-upb PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(upb_generator_protoc-gen-upb PROPERTIES OUTPUT_NAME protoc-gen-upb EXPORT_NAME protoc-gen-upb)
target_link_libraries(upb_generator_protoc-gen-upb PRIVATE
  upb_base
  upb_mem
  upb_mini_table_internal
  upb_port
  upb_wire_types
  absl::flat_hash_map
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_protoc-gen-upb PRIVATE
  upb_generator_common
  upb_generator_file_layout
  upb_generator_names
  upb_generator_plugin
  upb_generator_plugin_upb_proto
  upb_reflection_descriptor_upb_proto
  upb_reflection_reflection)
endif()
if (UPB_ENABLE_CODEGEN)

add_executable(upb_generator_protoc-gen-upb_minitable_stage0
    ../../upb_generator/protoc-gen-upb_minitable.cc
)
target_include_directories(upb_generator_protoc-gen-upb_minitable_stage0 PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_protoc-gen-upb_minitable_stage0 PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(upb_generator_protoc-gen-upb_minitable_stage0 PROPERTIES OUTPUT_NAME protoc-gen-upb_minitable_stage0 EXPORT_NAME protoc-gen-upb_minitable_stage0)
target_link_libraries(upb_generator_protoc-gen-upb_minitable_stage0 PRIVATE
  upb_base
  upb_mem
  upb_mini_table
  upb_mini_table_internal
  upb_port
  upb_wire_types
  absl::flat_hash_map
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_protoc-gen-upb_minitable_stage0 PRIVATE
  upb_generator_common_stage0
  upb_generator_file_layout_stage0
  upb_generator_names_stage0
  upb_generator_plugin_stage0
  upb_generator_plugin_upb_proto_stage0
  upb_reflection_descriptor_upb_proto_stage0
  upb_reflection_reflection_stage0)

add_executable(upb_generator_protoc-gen-upb_minitable_stage1
    ../../upb_generator/protoc-gen-upb_minitable.cc
)
target_include_directories(upb_generator_protoc-gen-upb_minitable_stage1 PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_protoc-gen-upb_minitable_stage1 PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(upb_generator_protoc-gen-upb_minitable_stage1 PROPERTIES OUTPUT_NAME protoc-gen-upb_minitable_stage1 EXPORT_NAME protoc-gen-upb_minitable_stage1)
target_link_libraries(upb_generator_protoc-gen-upb_minitable_stage1 PRIVATE
  upb_base
  upb_mem
  upb_mini_table
  upb_mini_table_internal
  upb_port
  upb_wire_types
  absl::flat_hash_map
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_protoc-gen-upb_minitable_stage1 PRIVATE
  upb_generator_common_stage1
  upb_generator_file_layout_stage1
  upb_generator_names_stage1
  upb_generator_plugin_stage1
  upb_generator_plugin_upb_proto_stage1
  upb_reflection_descriptor_upb_proto_stage1
  upb_reflection_reflection_stage1)

add_executable(upb_generator_protoc-gen-upb_minitable
    ../../upb_generator/protoc-gen-upb_minitable.cc
)
target_include_directories(upb_generator_protoc-gen-upb_minitable PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_protoc-gen-upb_minitable PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(upb_generator_protoc-gen-upb_minitable PROPERTIES OUTPUT_NAME protoc-gen-upb_minitable EXPORT_NAME protoc-gen-upb_minitable)
target_link_libraries(upb_generator_protoc-gen-upb_minitable PRIVATE
  upb_base
  upb_mem
  upb_mini_table
  upb_mini_table_internal
  upb_port
  upb_wire_types
  absl::flat_hash_map
  absl::flat_hash_set
  absl::absl_check
  absl::absl_log
  absl::strings)
target_link_libraries(upb_generator_protoc-gen-upb_minitable PRIVATE
  upb_generator_common
  upb_generator_file_layout
  upb_generator_names
  upb_generator_plugin
  upb_generator_plugin_upb_proto
  upb_reflection_descriptor_upb_proto
  upb_reflection_reflection)
endif()
if (UPB_ENABLE_CODEGEN)

add_executable(upb_generator_protoc-gen-upbdefs
    ../../upb_generator/protoc-gen-upbdefs.cc
)
target_include_directories(upb_generator_protoc-gen-upbdefs PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(upb_generator_protoc-gen-upbdefs PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(upb_generator_protoc-gen-upbdefs PROPERTIES OUTPUT_NAME protoc-gen-upbdefs EXPORT_NAME protoc-gen-upbdefs)
target_link_libraries(upb_generator_protoc-gen-upbdefs PRIVATE
  upb_generator_common
  upb_generator_file_layout
  upb_generator_plugin
  upb_descriptor_upb_proto
  upb_reflection
  upb_util_def_to_proto)
endif()

add_library(lua_lupb 
    ../../lua/def.c
  ../../lua/msg.c
  ../../lua/upb.c
  ../../lua/upb.h
)
target_include_directories(lua_lupb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
    $<INSTALL_INTERFACE:include>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(lua_lupb PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
target_link_libraries(lua_lupb PUBLIC
  upb_json
  upb_message
  upb_reflection
  upb_text
  @lua___liblua)
if (UPB_ENABLE_CODEGEN)

add_executable(lua_protoc-gen-lua
    ../../lua/upbc.cc
)
target_include_directories(lua_protoc-gen-lua PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
if(NOT UPB_ENABLE_CODEGEN)
  target_include_directories(lua_protoc-gen-lua PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
  )
endif()
set_target_properties(lua_protoc-gen-lua PROPERTIES OUTPUT_NAME protoc-gen-lua EXPORT_NAME protoc-gen-lua)
target_link_libraries(lua_protoc-gen-lua PRIVATE
  protobuf::libprotoc
  absl::strings)
endif()


if (UPB_ENABLE_CODEGEN)
  set(PROTOC_PROGRAM "\$<TARGET_FILE:protobuf::protoc>")
  set(PROTOC_GEN_UPB_PROGRAM "\$<TARGET_FILE:upb_generator_protoc-gen-upb>")
  set(PROTOC_GEN_UPB_MINITABLE_PROGRAM "\$<TARGET_FILE:upb_generator_protoc-gen-upb_minitable>")
  set(PROTOC_GEN_UPBDEFS_PROGRAM "\$<TARGET_FILE:upb_generator_protoc-gen-upbdefs>")
  set(PROTOC_GEN_UPBLUA_PROGRAM "\$<TARGET_FILE:lua_protoc-gen-lua>")

  unset(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_LUAS)
  unset(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS)
  unset(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_SOURCES)
  unset(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_FILES)
  set(UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_NAMES any api duration empty
      field_mask source_context struct timestamp type wrappers)
  foreach(PROTO_NAME IN LISTS UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_NAMES)
    list(APPEND UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_FILES
          "${UPB_HOST_INCLUDE_DIR}/google/protobuf/${PROTO_NAME}.proto")
    list(APPEND UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_LUAS
          "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}_pb.lua")
    list(APPEND UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS
          "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upb.h"
          "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upb_minitable.h"
          "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upbdefs.h")
    list(APPEND UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_SOURCES
          "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upb.c"
          "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upb_minitable.c"
          "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf/${PROTO_NAME}.upbdefs.c")
  endforeach()
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types/google/protobuf")
  add_custom_command(
    OUTPUT ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_LUAS}
          ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS}
          ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_SOURCES}
    DEPENDS ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_FILES}
    COMMAND
      "${PROTOC_PROGRAM}"
      "-I${UPB_HOST_INCLUDE_DIR}"
      "--plugin=protoc-gen-upb=${PROTOC_GEN_UPB_PROGRAM}"
      "--plugin=protoc-gen-upb_minitable=${PROTOC_GEN_UPB_MINITABLE_PROGRAM}"
      "--plugin=protoc-gen-upbdefs=${PROTOC_GEN_UPBDEFS_PROGRAM}"
      "--plugin=protoc-gen-lua=${PROTOC_GEN_UPBLUA_PROGRAM}"
      "--upb_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types"
      "--upb_minitable_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types"
      "--upbdefs_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types"
      "--lua_out=${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types"
      ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_PROTO_FILES}
  )
  add_library(upb_well_known_types ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS}
    ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_SOURCES})
  target_include_directories(upb_well_known_types PUBLIC "\$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/stage2/upb_well_known_types>")
  set_target_properties(upb_well_known_types PROPERTIES EXPORT_NAME "well_known_types")
  target_link_libraries(upb_well_known_types PUBLIC upb_upb upb_descriptor_upb_proto)
endif()

include(GNUInstallDirs)
install(
  DIRECTORY ../../upb
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.hpp"
  PATTERN "*.inc"
)
install(TARGETS
  upb_base_base upb_base_internal upb_collections_collections upb_hash_hash upb_io_string upb_io_tokenizer upb_io_zero_copy_stream upb_json_json upb_lex_lex upb_mem_mem upb_mem_internal upb_message_accessors upb_message_accessors_internal upb_message_copy upb_message_internal upb_message_internal_types upb_message_message upb_message_promote upb_message_split64 upb_message_tagged_ptr upb_message_types upb_message_value upb_mini_descriptor_mini_descriptor upb_mini_descriptor_internal upb_mini_table_compat upb_mini_table_mini_table upb_mini_table_internal upb_port_port upb_reflection_generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me upb_text_text upb_util_def_to_proto upb_util_required_fields upb_util_compare upb_wire_wire upb_wire_internal upb_wire_reader upb_wire_types upb_wire_eps_copy_input_stream upb_upb upb_generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me upb_generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me upb_generator_keywords upb_generator_mangle lua_lupb
  EXPORT upb-config
)
if (UPB_ENABLE_CODEGEN)
  install(
    FILES
      ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_LUAS}
      ${UPB_DESCRIPTOR_UPB_WELL_KNOWN_TYPES_HEADERS}
    DESTINATION include/google/protobuf
  )
  install(
    DIRECTORY ../../lua/
    DESTINATION share/upb/lua
  )
  install(TARGETS
    upb_well_known_types
    upb_reflection_descriptor_upb_proto upb_reflection_descriptor_upb_proto_minitable upb_reflection_descriptor_upb_proto_defs upb_reflection_reflection upb_reflection_internal upb_generator_plugin_upb_proto upb_generator_plugin_upb_proto_minitable upb_generator_plugin_upb_proto_defs upb_generator_common upb_generator_file_layout upb_generator_plugin upb_generator_names upb_generator_protoc-gen-upb upb_generator_protoc-gen-upb_minitable upb_generator_protoc-gen-upbdefs lua_protoc-gen-lua
    ${UPB_CODEGEN_TARGETS}
    EXPORT upb-config
  )
endif()
install(EXPORT upb-config NAMESPACE protobuf:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/upb")
