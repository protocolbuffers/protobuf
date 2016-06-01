# Library aliases
foreach(_library
  libprotobuf-lite
  libprotobuf
  libprotoc)
  if(TARGET ${_library} AND NOT TARGET protobuf::${_library})
    add_library(protobuf::${_library} ALIAS ${_library})
  endif()
endforeach()

# Executable aliases
foreach(_executable
  protoc)
  if(TARGET ${_executable} AND NOT TARGET protobuf::${_executable})
    add_executable(protobuf::${_executable} ALIAS ${_executable})
  endif()
endforeach()
