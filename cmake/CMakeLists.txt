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
include_directories(../third_party/utf8_range)
include_directories(../external/utf8_range)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(APPLE)
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined dynamic_lookup -flat_namespace")
elseif(UNIX)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--build-id")
endif()

enable_testing()

add_library(port INTERFACE)
add_library(upb
  ../upb/arena.c
  ../upb/alloc.c
  ../upb/array.c
  ../upb/decode.c
  ../upb/encode.c
  ../upb/internal/table.h
  ../upb/internal/unicode.h
  ../upb/map_sorter.c
  ../upb/map_sorter.h
  ../upb/msg.c
  ../upb/msg_internal.h
  ../upb/status.c
  ../upb/upb.c
  ../upb/alloc.h
  ../upb/arena.h
  ../upb/array.h
  ../upb/decode.h
  ../upb/encode.h
  ../upb/extension_registry.h
  ../upb/message_value.h
  ../upb/msg.h
  ../upb/status.h
  ../upb/string_view.h
  ../upb/upb.h
  ../upb/upb.hpp)
target_link_libraries(upb
  arena_internal
  array_internal
  decode_internal
  encode_internal
  extension_registry
  fastdecode
  port
  table_internal
  unicode_internal)
add_library(extension_registry
  ../upb/extension_registry.c
  ../upb/msg.h
  ../upb/msg_internal.h
  ../upb/string_view.h
  ../upb/upb.h
  ../upb/extension_registry.h)
target_link_libraries(extension_registry
  port
  table_internal)
add_library(mini_table_internal INTERFACE)
target_link_libraries(mini_table_internal INTERFACE
  extension_registry
  port
  table_internal
  upb)
add_library(mini_table
  ../upb/mini_table.c
  ../upb/mini_table.h
  ../upb/mini_table.hpp)
target_link_libraries(mini_table
  extension_registry
  mini_table_internal
  port
  upb)
add_library(mini_table_accessors
  ../upb/internal/mini_table_accessors.h
  ../upb/mini_table_accessors.c
  ../upb/mini_table_accessors.h)
target_link_libraries(mini_table_accessors
  array_internal
  collections
  mini_table
  mini_table_internal
  port
  upb)
add_library(fastdecode
  ../upb/decode.h
  ../upb/decode_fast.c
  ../upb/decode_fast.h
  ../upb/msg.h
  ../upb/msg_internal.h)
target_link_libraries(fastdecode
  arena_internal
  array_internal
  decode_internal
  extension_registry
  port
  table_internal)
add_library(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE)
target_link_libraries(generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  table_internal
  upb)
add_library(generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE)
target_link_libraries(generated_cpp_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  mini_table
  table_internal
  upb)
add_library(generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE)
target_link_libraries(generated_reflection_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me INTERFACE
  descriptor_upb_proto
  reflection
  reflection_internal
  table_internal)
add_library(collections
  ../upb/map.c
  ../upb/msg_internal.h
  ../upb/collections.h
  ../upb/map.h
  ../upb/map_sorter.h
  ../upb/message_value.h)
target_link_libraries(collections
  mini_table
  port
  table_internal
  upb)
add_library(reflection
  ../upb/reflection/common.h
  ../upb/reflection/def_builder.c
  ../upb/reflection/def_builder_internal.h
  ../upb/reflection/def_pool.c
  ../upb/reflection/def_pool.h
  ../upb/reflection/def_pool_internal.h
  ../upb/reflection/def_type.c
  ../upb/reflection/def_type.h
  ../upb/reflection/desc_state.c
  ../upb/reflection/desc_state_internal.h
  ../upb/reflection/enum_def.c
  ../upb/reflection/enum_def.h
  ../upb/reflection/enum_def_internal.h
  ../upb/reflection/enum_value_def.c
  ../upb/reflection/enum_value_def.h
  ../upb/reflection/enum_value_def_internal.h
  ../upb/reflection/extension_range.c
  ../upb/reflection/extension_range.h
  ../upb/reflection/extension_range_internal.h
  ../upb/reflection/field_def.c
  ../upb/reflection/field_def.h
  ../upb/reflection/field_def_internal.h
  ../upb/reflection/file_def.c
  ../upb/reflection/file_def.h
  ../upb/reflection/file_def_internal.h
  ../upb/reflection/message.c
  ../upb/reflection/message_def.c
  ../upb/reflection/message_def.h
  ../upb/reflection/message_def_internal.h
  ../upb/reflection/method_def.c
  ../upb/reflection/method_def.h
  ../upb/reflection/method_def_internal.h
  ../upb/reflection/oneof_def.c
  ../upb/reflection/oneof_def.h
  ../upb/reflection/oneof_def_internal.h
  ../upb/reflection/service_def.c
  ../upb/reflection/service_def.h
  ../upb/reflection/service_def_internal.h
  ../upb/def.h
  ../upb/def.hpp
  ../upb/reflection.h
  ../upb/reflection.hpp
  ../upb/reflection/def.h
  ../upb/reflection/def.hpp
  ../upb/reflection/message.h
  ../upb/reflection/message.hpp)
target_link_libraries(reflection
  collections
  descriptor_upb_proto
  mini_table
  mini_table_internal
  port
  table_internal
  upb)
add_library(reflection_internal INTERFACE)
target_link_libraries(reflection_internal INTERFACE
  collections
  descriptor_upb_proto
  mini_table
  mini_table_internal
  port
  reflection
  table_internal
  upb)
add_library(textformat
  ../upb/text_encode.c
  ../upb/text_encode.h)
target_link_libraries(textformat
  collections
  encode_internal
  port
  reflection)
add_library(json
  ../upb/json/decode.c
  ../upb/json/encode.c
  ../upb/json/decode.h
  ../upb/json/encode.h
  ../upb/json_decode.h
  ../upb/json_encode.h)
target_link_libraries(json
  atoi_internal
  collections
  encode_internal
  port
  reflection
  unicode_internal
  upb)
add_library(array_internal
  ../upb/alloc.h
  ../upb/arena.h
  ../upb/array.h
  ../upb/internal/array.c
  ../upb/message_value.h
  ../upb/msg.h
  ../upb/status.h
  ../upb/string_view.h
  ../upb/upb.h
  ../upb/internal/array.h)
target_link_libraries(array_internal
  port)
add_library(arena_internal INTERFACE)
target_link_libraries(arena_internal INTERFACE
  port)
add_library(atoi_internal
  ../upb/internal/atoi.c
  ../upb/internal/atoi.h)
target_link_libraries(atoi_internal
  port)
add_library(decode_internal INTERFACE)
target_link_libraries(decode_internal INTERFACE
  arena_internal
  port
  table_internal
  utf8_range)
add_library(encode_internal INTERFACE)
target_link_libraries(encode_internal INTERFACE
  port)
add_library(table_internal
  ../upb/internal/table.c
  ../upb/alloc.h
  ../upb/arena.h
  ../upb/internal/table.h
  ../upb/status.h
  ../upb/string_view.h
  ../upb/upb.h)
target_link_libraries(table_internal
  port)
add_library(unicode_internal
  ../upb/internal/unicode.c
  ../upb/internal/unicode.h)
target_link_libraries(unicode_internal
  port)


