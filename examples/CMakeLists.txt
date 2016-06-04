# Minimum CMake required
cmake_minimum_required(VERSION 2.8.12)

# Project
project(protobuf-examples)

# Find required protobuf package
find_package(protobuf CONFIG REQUIRED)

if(protobuf_VERBOSE)
  message(STATUS "Using Protocol Buffers ${Protobuf_VERSION}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime
if(MSVC AND protobuf_MSVC_STATIC_RUNTIME)
  foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach()
endif()

foreach(example add_person list_people)
  set(${example}_SRCS ${example}.cc)
  set(${example}_PROTOS addressbook.proto)
  set(executable_name ${example}_cpp)

  # Legacy Support - set protobuf_MODULE_COMPATIBLE to allow compatibility
  # with the CMake FindProbuf module.
  if(protobuf_MODULE_COMPATIBLE)
    protobuf_generate_cpp(${example}_PROTO_SRCS ${example}_PROTO_HDRS ${${example}_PROTOS})
    list(APPEND ${example}_SRCS ${${example}_PROTO_SRCS} ${${example}_PROTO_HDRS})

    add_executable(${executable_name} ${${example}_SRCS} ${${example}_PROTOS})
	  target_include_directories(${executable_name} PUBLIC ${PROTOBUF_INCLUDE_DIRS})
    target_link_libraries(${executable_name} ${PROTOBUF_LIBRARIES})
  else()
    list(APPEND ${example}_SRCS ${${example}_PROTOS})

    if(CMAKE_MAJOR_VERSION LESS 3)
      protobuf_generate(${example}_SRCS)
      add_executable(${executable_name} ${${example}_SRCS})
    else()
      add_executable(${executable_name} ${${example}_SRCS})
      protobuf_generate(${executable_name})
    endif()

    target_link_libraries(${executable_name} protobuf::libprotobuf)
  endif()
endforeach()
