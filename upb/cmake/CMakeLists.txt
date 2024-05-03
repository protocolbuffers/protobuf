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
  if(EXISTS ../../external/utf8_range)
    # utf8_range is already installed
    include_directories(../external/utf8_range)
  elseif(EXISTS ../../utf8_range)
    include_directories(../../utf8_range)
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

enable_testing()


add_library(upb INTERFACE
    
)
target_include_directories(upb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(upb INTERFACE
  base
  mem)

add_library(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)
target_link_libraries(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  base
  mem
  message
  message_accessors
  message_accessors_internal
  message_internal
  mini_descriptor
  mini_table
  wire
  wire_internal)

add_library(generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    
)
target_include_directories(generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../cmake>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINRARY_DIR}>
)

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


