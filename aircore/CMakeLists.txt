# build libprotobuf
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0077 NEW)

set(protobuf_ABSL_PROVIDER "none" CACHE STRING "")
add_subdirectory_once(${at_media_deps_dir}/abseil-cpp ${CMAKE_BINARY_DIR}/abseil-cpp)
set(protobuf_INSTALL NO)

set(INCLUDE_LIBPROTOBUF 1)

set(protobuf_WITH_ZLIB OFF CACHE INTERNAL "")
set(protobuf_BUILD_TESTS OFF CACHE INTERNAL "")
if (ANDROID OR IOS)
  set(BUILD_PROTOC_EXE OFF CACHE INTERNAL "")
endif()
add_subdirectory(${at_media_deps_dir}/protobuf ${CMAKE_BINARY_DIR}/protobuf/src/cmake)

# On Clang 9 (Starting Android NDK r21b) this is nesserary to compile.
# (Error later seen on Xcode 12 too).
target_compile_options(libprotobuf PRIVATE -Wno-enum-compare-switch)

# We don't build libprotoc for android or ios
if (TARGET libprotoc)
  target_compile_options(libprotoc PRIVATE -Wno-enum-compare-switch)
endif()

# On iOS and Android we need to make sure we use the host version of protoc to generate files
set(PROTOBUF_PROTOC_EXECUTABLE "")
if (IOS)
  set(PROTOBUF_PROTOC_EXECUTABLE 
"${CMAKE_BINARY_DIR}/../xcode-osx/protobuf/src/cmake/Release/protoc")
elseif(ANDROID)
  set(PROTOBUF_PROTOC_EXECUTABLE 
"${CMAKE_BINARY_DIR}/../${at_host_os}-${at_host_arch}-release/protobuf/src/cmake/protoc")
else()
  set(PROTOBUF_PROTOC_EXECUTABLE $<TARGET_FILE:protoc>)
endif()

# We don't build protoc for android or ios, but we call 
# set_target_properties on
# the protoc target below, so we need to create a stub target
if (NOT TARGET protoc)
  add_custom_target(protoc)
endif()

if (PROTOBUF_PROTOC_EXECUTABLE)
  set_target_properties(protoc PROPERTIES TARGET_FILE 
${PROTOBUF_PROTOC_EXECUTABLE})
endif()

set(protoc_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Copy the protoc binary to the protobuf src directory since
# 'mvn package' expects it there
add_custom_target(protoc-symlink
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${PROTOBUF_PROTOC_EXECUTABLE}
  ${protoc_source_dir}/src/protoc
)

set(protoc_java_dir ${protoc_source_dir}/java)

add_custom_target(protoc-jar
  COMMAND mvn clean
  COMMAND mvn package
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${protoc_java_dir}/core/target/protobuf-java*.jar
          ${CMAKE_BINARY_DIR}/protobuf
  WORKING_DIRECTORY ${protoc_java_dir}
  DEPENDS protoc-symlink
)
