# Minimum CMake required
cmake_minimum_required(VERSION 2.8.12)

# Project
project(addressbook)

# Find required protobuf package
find_package(protobuf 3 CONFIG REQUIRED)

if(protobuf_VERBOSE)
  message(STATUS "Using Protocol Buffers ${Protobuf_VERSION}")
endif()

file(GLOB SRC_LIST "*.cc")

file(GLOB PROTO_LIST "*.proto")

#
# Code generation
#

if(protobuf_MODULE_COMPATIBLE) # Old school

  include_directories(${Protobuf_INCLUDE_DIRS})
  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_LIST})

else() # New style

  set(PROTO_SRCS)
  set(PROTO_HDRS)

  foreach(FIL ${PROTO_LIST})

    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)

    list(APPEND PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")
    list(APPEND PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc"
             "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.h"
      COMMAND protobuf::protoc
      ARGS --cpp_out ${CMAKE_CURRENT_BINARY_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR}
        ${ABS_FIL}
      DEPENDS ${ABS_FIL}
      COMMENT "Running C++ protocol buffer compiler on ${FIL}"
      VERBATIM)

  endforeach()

endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR})

include(GNUInstallDirs)

foreach(FIL ${SRC_LIST})

  get_filename_component(FIL_NAME_WE ${FIL} NAME_WE)

  set(APP ${FIL_NAME_WE}_cpp)

  add_executable(${APP} ${FIL} ${PROTO_SRCS} ${PROTO_HDRS})

  #
  # Link libraries
  #

  if(protobuf_MODULE_COMPATIBLE) # Old school
    target_link_libraries(${APP} ${Protobuf_LIBRARIES})
  else() # New style
    target_link_libraries(${APP} protobuf::libprotobuf)
  endif()

  install(TARGETS ${APP}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${APP})

endforeach()
