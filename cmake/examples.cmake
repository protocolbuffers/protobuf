if(protobuf_VERBOSE)
  message(STATUS "Protocol Buffers Examples Configuring...")
endif()

# Add examples subproject
add_subdirectory(../examples examples)

if(protobuf_VERBOSE)
  message(STATUS "Protocol Buffers Examples Configuring done")
endif()
