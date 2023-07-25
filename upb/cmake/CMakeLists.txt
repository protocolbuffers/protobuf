# This file was generated from BUILD using tools/make_cmakelists.py.

cmake_minimum_required(VERSION 3.10...3.24)

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

if(NOT TARGET utf8_range)
  if(EXISTS ../external/utf8_range)
    # utf8_range is already installed
    include_directories(../external/utf8_range)
  else()
    include(FetchContent)
    FetchContent_Declare(
      utf8_range
      GIT_REPOSITORY "https://github.com/protocolbuffers/utf8_range.git"
      GIT_TAG "de0b4a8ff9b5d4c98108bdfe723291a33c52c54f"
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

enable_testing()


add_library(port INTERFACE
    
)
target_include_directories(port INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)

add_library(upb INTERFACE
    
)
target_include_directories(upb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(upb INTERFACE
  base
  collections_internal
  fastdecode
  hash
  lex
  mem
  message_internal
  port
  wire
  /upb/mini_table)

add_library(base 
    ../upb/base/status.c
  ../upb/base/descriptor_constants.h
  ../upb/base/log2.h
  ../upb/base/status.h
  ../upb/base/string_view.h
)
target_include_directories(base PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(base
  port)

add_library(message INTERFACE
    
)
target_include_directories(message INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message INTERFACE
  mem
  message_internal
  port
  /upb/mini_table)

add_library(message_internal 
    ../upb/message/message.c
  ../upb/message/extension_internal.h
  ../upb/message/internal.h
  ../upb/message/message.h
)
target_include_directories(message_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_internal
  base
  hash
  mem
  port
  /upb/mini_table)

add_library(message_tagged_ptr INTERFACE
    
)
target_include_directories(message_tagged_ptr INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_tagged_ptr INTERFACE
  port)

add_library(message_accessors_internal INTERFACE
    
)
target_include_directories(message_accessors_internal INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_accessors_internal INTERFACE
  collections_internal
  message_internal
  mini_table_internal
  port)

add_library(message_accessors 
    ../upb/message/accessors.c
  ../upb/message/accessors_internal.h
  ../upb/message/accessors.h
)
target_include_directories(message_accessors PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_accessors
  collections_internal
  eps_copy_input_stream
  message_internal
  mini_table
  port
  upb
  wire
  wire_reader
  /upb/mini_table:internal)

add_library(message_promote 
    ../upb/message/promote.c
  ../upb/message/promote.h
)
target_include_directories(message_promote PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_promote
  collections_internal
  eps_copy_input_stream
  hash
  message_accessors
  message_internal
  port
  upb
  wire
  wire_reader
  /upb/mini_table)

add_library(message_copy 
    ../upb/message/copy.c
  ../upb/message/copy.h
)
target_include_directories(message_copy PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_copy
  collections_internal
  message_accessors
  message_internal
  port
  upb
  /upb/mini_table)

add_library(message_split64 INTERFACE
    
)
target_include_directories(message_split64 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_split64 INTERFACE
  message_accessors
  port)

add_library(fastdecode INTERFACE
    
)
target_include_directories(fastdecode INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(fastdecode INTERFACE
  base
  collections_internal
  hash
  mem_internal
  message_internal
  port
  wire
  /upb/mini_table)

add_library(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  collections_internal
  message_accessors
  message_accessors_internal
  message_internal
  upb
  wire_internal
  /upb/mini_descriptor
  /upb/mini_table)

add_library(generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  base
  collections_internal
  hash
  message_copy
  upb
  /upb/mini_table)

add_library(generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  reflection_internal
  /upb/mini_descriptor)

add_library(collections INTERFACE
    
)
target_include_directories(collections INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(collections INTERFACE
  base
  collections_internal
  mem
  port)

add_library(message_rep_internal INTERFACE
    
)
target_include_directories(message_rep_internal INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_rep_internal INTERFACE
  base
  hash)

add_library(collections_internal 
    ../upb/collections/array.c
  ../upb/collections/map.c
  ../upb/collections/map_sorter.c
  ../upb/collections/array.h
  ../upb/collections/array_internal.h
  ../upb/collections/map.h
  ../upb/collections/map_gencode_util.h
  ../upb/collections/map_internal.h
  ../upb/collections/map_sorter_internal.h
  ../upb/collections/message_value.h
)
target_include_directories(collections_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(collections_internal
  base
  hash
  mem
  message_internal
  message_rep_internal
  message_tagged_ptr
  port
  /upb/mini_table)

add_library(collections_split64 INTERFACE
    
)
target_include_directories(collections_split64 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(collections_split64 INTERFACE
  collections
  port)

add_library(textformat 
    ../upb/text/encode.c
  ../upb/text/encode.h
  ../upb/text_encode.h
)
target_include_directories(textformat PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(textformat
  collections_internal
  eps_copy_input_stream
  lex
  port
  reflection
  wire
  wire_reader
  wire_types)

add_library(mem INTERFACE
    
)
target_include_directories(mem INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(mem INTERFACE
  mem_internal
  port)

add_library(mem_internal 
    ../upb/mem/alloc.c
  ../upb/mem/arena.c
  ../upb/mem/alloc.h
  ../upb/mem/arena.h
  ../upb/mem/arena_internal.h
)
target_include_directories(mem_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(mem_internal
  port)

add_library(wire INTERFACE
    
)
target_include_directories(wire INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(wire INTERFACE
  mem
  message_internal
  port
  wire_internal
  /upb/mini_table)

add_library(wire_internal 
    ../upb/wire/decode.c
  ../upb/wire/decode_fast.c
  ../upb/wire/encode.c
  ../upb/wire/common.h
  ../upb/wire/common_internal.h
  ../upb/wire/decode.h
  ../upb/wire/decode_fast.h
  ../upb/wire/decode_internal.h
  ../upb/wire/encode.h
  ../upb/wire/swap_internal.h
)
target_include_directories(wire_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(wire_internal
  base
  collections_internal
  eps_copy_input_stream
  mem_internal
  message_accessors_internal
  message_internal
  message_rep_internal
  port
  wire_reader
  wire_types
  /upb/mini_table
  utf8_range)

add_library(wire_types INTERFACE
    
)
target_include_directories(wire_types INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)

add_library(eps_copy_input_stream 
    ../upb/wire/eps_copy_input_stream.c
  ../upb/wire/eps_copy_input_stream.h
)
target_include_directories(eps_copy_input_stream PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(eps_copy_input_stream
  mem
  port)

add_library(wire_reader 
    ../upb/wire/reader.c
  ../upb/wire/swap_internal.h
  ../upb/wire/reader.h
)
target_include_directories(wire_reader PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(wire_reader
  eps_copy_input_stream
  port
  wire_types)

add_library(hash 
    ../upb/hash/common.c
  ../upb/hash/common.h
  ../upb/hash/int_table.h
  ../upb/hash/str_table.h
)
target_include_directories(hash PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(hash
  base
  mem
  port)

add_library(lex 
    ../upb/lex/atoi.c
  ../upb/lex/round_trip.c
  ../upb/lex/strtod.c
  ../upb/lex/unicode.c
  ../upb/lex/atoi.h
  ../upb/lex/round_trip.h
  ../upb/lex/strtod.h
  ../upb/lex/unicode.h
)
target_include_directories(lex PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(lex
  port)


