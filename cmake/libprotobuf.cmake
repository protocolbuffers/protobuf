# CMake definitions for libprotobuf (the "full" C++ protobuf runtime).

include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)

add_library(libprotobuf ${protobuf_SHARED_OR_STATIC}
  ${libprotobuf_srcs}
  ${libprotobuf_hdrs}
  ${protobuf_version_rc_file})
if(protobuf_HAVE_LD_VERSION_SCRIPT)
  if(${CMAKE_VERSION} VERSION_GREATER 3.13 OR ${CMAKE_VERSION} VERSION_EQUAL 3.13)
    target_link_options(libprotobuf PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotobuf.map)
  elseif(protobuf_BUILD_SHARED_LIBS)
    target_link_libraries(libprotobuf PRIVATE -Wl,--version-script=${protobuf_SOURCE_DIR}/src/libprotobuf.map)
  endif()
  set_target_properties(libprotobuf PROPERTIES
    LINK_DEPENDS ${protobuf_SOURCE_DIR}/src/libprotobuf.map)
endif()
target_link_libraries(libprotobuf PRIVATE ${CMAKE_THREAD_LIBS_INIT})
if(protobuf_WITH_ZLIB)
  target_link_libraries(libprotobuf PRIVATE ${ZLIB_LIBRARIES})
endif()
if(protobuf_LINK_LIBATOMIC)
  target_link_libraries(libprotobuf PRIVATE atomic)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
  target_link_libraries(libprotobuf PRIVATE log)
endif()
target_include_directories(libprotobuf PUBLIC ${protobuf_SOURCE_DIR}/src)
target_link_libraries(libprotobuf PUBLIC ${protobuf_ABSL_USED_TARGETS})
if(protobuf_BUILD_SHARED_LIBS)
  target_compile_definitions(libprotobuf
    PUBLIC  PROTOBUF_USE_DLLS
    PRIVATE LIBPROTOBUF_EXPORTS)
endif()
set_target_properties(libprotobuf PROPERTIES
    VERSION ${protobuf_VERSION}
    OUTPUT_NAME ${LIB_PREFIX}protobuf
    DEBUG_POSTFIX "${protobuf_DEBUG_POSTFIX}"
    # For -fvisibility=hidden and -fvisibility-inlines-hidden
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
add_library(protobuf::libprotobuf ALIAS libprotobuf)

target_link_libraries(libprotobuf PRIVATE utf8_validity)
