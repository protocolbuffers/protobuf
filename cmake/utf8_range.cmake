if (NOT TARGET utf8_range)
  set(utf8_range_ENABLE_TESTS OFF CACHE BOOL "Disable utf8_range tests")

  if (NOT EXISTS "${protobuf_SOURCE_DIR}/third_party/utf8_range/CMakeLists.txt")
    message(FATAL_ERROR
            "Cannot find third_party/utf8_range directory that's needed for "
            "the protobuf runtime.\n")
  endif()

  set(utf8_range_ENABLE_INSTALL ${protobuf_INSTALL} CACHE BOOL "Set install")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/utf8_range third_party/utf8_range)
  include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/utf8_range)
endif ()

set(_protobuf_FIND_UTF8_RANGE "if(NOT TARGET utf8_range)\n  find_package(utf8_range CONFIG)\nendif()")
