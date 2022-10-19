set(utf8_range_ENABLE_TESTS OFF)

if (NOT EXISTS "${protobuf_SOURCE_DIR}/third_party/utf8_range/CMakeLists.txt")
  message(FATAL_ERROR
          "Cannot find third_party/utf8_range directory that's needed to "
          "build conformance tests. If you use git, make sure you have cloned "
          "submodules:\n"
          "  git submodule update --init --recursive\n")
endif()

set(utf8_range_ENABLE_INSTALL ${protobuf_INSTALL})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/utf8_range third_party/utf8_range)