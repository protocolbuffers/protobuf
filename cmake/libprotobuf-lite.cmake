# CMake definitions for libprotobuf_lite (the "lite" C++ protobuf runtime).

include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)
include(${protobuf_SOURCE_DIR}/cmake/protobuf-configure-target.cmake)

add_library(libprotobuf-lite ${protobuf_SHARED_OR_STATIC}
  ${libprotobuf_lite_srcs}
  ${libprotobuf_lite_hdrs}
  ${protobuf_version_rc_file})
if(protobuf_HAVE_LD_VERSION_SCRIPT)
  if(${CMAKE_VERSION} VERSION_GREATER 3.13 OR ${CMAKE_VERSION} VERSION_EQUAL 3.13)
    target_link_options(libprotobuf-lite PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotobuf-lite.map)
  elseif(protobuf_BUILD_SHARED_LIBS)
    target_link_libraries(libprotobuf-lite PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotobuf-lite.map)
  endif()
  set_target_properties(libprotobuf-lite PROPERTIES
    LINK_DEPENDS ${protobuf_SOURCE_DIR}/src/libprotobuf-lite.map)
endif()
target_link_libraries(libprotobuf-lite PRIVATE ${CMAKE_THREAD_LIBS_INIT})
if(protobuf_LINK_LIBATOMIC)
  target_link_libraries(libprotobuf-lite PRIVATE atomic)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  target_link_libraries(libprotobuf-lite PRIVATE log)
endif()
target_include_directories(libprotobuf-lite PUBLIC ${protobuf_SOURCE_DIR}/src)
target_link_libraries(libprotobuf-lite PUBLIC ${protobuf_ABSL_USED_TARGETS})
protobuf_configure_target(libprotobuf-lite)
if(protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libprotobuf-lite
    PUBLIC  PROTOBUF_USE_DLLS
    PRIVATE LIBPROTOBUF_EXPORTS)
endif()
set_target_properties(libprotobuf-lite PROPERTIES
    VERSION ${protobuf_VERSION}
    OUTPUT_NAME ${LIB_PREFIX}protobuf-lite
    DEBUG_POSTFIX "${protobuf_DEBUG_POSTFIX}"
    # For -fvisibility=hidden and -fvisibility-inlines-hidden
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
add_library(protobuf::libprotobuf-lite ALIAS libprotobuf-lite)

target_link_libraries(libprotobuf-lite PRIVATE utf8_validity)
