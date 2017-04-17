set(protoc_files
  ${protobuf_source_dir}/src/google/protobuf/compiler/main.cc
)

# https://cmake.org/Wiki/CMake_Cross_Compiling#Using_executables_in_the_build_created_during_the_build
# when crosscompiling import the executable targets from a file
if(NOT CMAKE_CROSSCOMPILING)
  add_executable(protoc ${protoc_files})
  target_link_libraries(protoc libprotobuf libprotoc)
endif()