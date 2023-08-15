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
  elseif(EXISTS ../../utf8_range)
    include_directories(../../utf8_range)
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
  mem
  message_internal
  mini_table
  port)

add_library(message INTERFACE
    
)
target_include_directories(message INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message INTERFACE
  base
  mem
  message_internal
  mini_table
  port)

add_library(message_internal 
    ../upb/message/message.c
  ../upb/message/internal/extension.h
  ../upb/message/internal/message.h
  ../upb/message/message.h
)
target_include_directories(message_internal PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_internal
  base
  base_internal
  hash
  mem
  mini_table
  port)

add_library(message_tagged_ptr INTERFACE
    
)
target_include_directories(message_tagged_ptr INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_tagged_ptr INTERFACE
  message_typedef
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
  ../upb/message/internal/accessors.h
  ../upb/message/accessors.h
)
target_include_directories(message_accessors PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_accessors
  base
  collections_internal
  eps_copy_input_stream
  message_internal
  mini_table
  mini_table_internal
  port
  upb
  wire
  wire_internal
  wire_reader)

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
  base
  collections_internal
  eps_copy_input_stream
  hash
  mem
  message_accessors
  message_internal
  message_typedef
  mini_table
  port
  upb
  wire
  wire_internal
  wire_reader)

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
  base
  collections_internal
  mem
  message_accessors
  message_internal
  message_typedef
  mini_table
  mini_table_internal
  port
  upb)

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

add_library(message_typedef INTERFACE
    
)
target_include_directories(message_typedef INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(message_typedef INTERFACE
  )

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
  mini_table
  port
  wire)

add_library(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  base
  collections_internal
  mem
  message_accessors
  message_accessors_internal
  message_internal
  mini_descriptor
  mini_table
  upb
  wire_internal)

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
  mem
  message_copy
  message_typedef
  mini_table
  upb)

add_library(generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  mem
  mini_descriptor
  reflection_internal)

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


