# CMake definitions for libupb.

include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)
include(${protobuf_SOURCE_DIR}/cmake/protobuf-configure-target.cmake)

add_library(libupb ${protobuf_SHARED_OR_STATIC}
  ${libupb_srcs}
  ${libupb_hdrs}
  ${protobuf_version_rc_file})
if(protobuf_HAVE_LD_VERSION_SCRIPT)
  target_link_options(libupb PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libupb.map)
  set_target_properties(libupb PROPERTIES
    LINK_DEPENDS ${protobuf_SOURCE_DIR}/src/libupb.map)
endif()
target_include_directories(libupb PUBLIC ${protobuf_SOURCE_DIR}/src)
target_include_directories(libupb PUBLIC ${protobuf_SOURCE_DIR}/upb/reflection/stage1)
protobuf_configure_target(libupb)
if(protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libupb
    PUBLIC  UPB_BUILD_API
    PRIVATE LIBPROTOBUF_EXPORTS)
endif()
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
