# CMake definitions for libupb.

include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)
include(${protobuf_SOURCE_DIR}/cmake/protobuf-configure-target.cmake)

set(bootstrap_cmake_dir ${protobuf_SOURCE_DIR}/upb/reflection/cmake)
set(bootstrap_sources
  # Hardcode bootstrap paths
  ${bootstrap_cmake_dir}/google/protobuf/descriptor.upb.h
  ${bootstrap_cmake_dir}/google/protobuf/descriptor.upb_minitable.h
  ${bootstrap_cmake_dir}/google/protobuf/descriptor.upb_minitable.c
)

# Note: upb does not support shared library builds, and is intended to be
# statically linked as a private dependency.
add_library(libupb STATIC
  ${libupb_srcs}
  ${libupb_hdrs}
  ${bootstrap_sources}
  ${protobuf_version_rc_file}
)

target_include_directories(libupb PUBLIC
  $<BUILD_INTERFACE:${bootstrap_cmake_dir}>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

protobuf_configure_target(libupb)

set_target_properties(libupb PROPERTIES
    VERSION ${protobuf_VERSION}
    OUTPUT_NAME ${LIB_PREFIX}upb
    DEBUG_POSTFIX "${protobuf_DEBUG_POSTFIX}"
    # For -fvisibility=hidden and -fvisibility-inlines-hidden
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
add_library(protobuf::libupb ALIAS libupb)
target_link_libraries(libupb PRIVATE utf8_range)
