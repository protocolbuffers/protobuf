include(GNUInstallDirs)

foreach(_target IN LISTS protobuf_ABSL_USED_TARGETS)
  string(REPLACE :: _ _modified_target ${_target})
  list(APPEND _pc_targets ${_modified_target})
endforeach()
list(APPEND _pc_targets "utf8_range")

set(_protobuf_PC_REQUIRES "")
set(_sep "")
foreach (_target IN LISTS _pc_targets)
  string(CONCAT _protobuf_PC_REQUIRES "${_protobuf_PC_REQUIRES}" "${_sep}" "${_target}")
  set(_sep " ")
endforeach ()
set(_protobuf_PC_CFLAGS)
if (protobuf_BUILD_SHARED_LIBS)
  set(_protobuf_PC_CFLAGS -DPROTOBUF_USE_DLLS)
endif ()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/protobuf.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/protobuf.pc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/protobuf-lite.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/protobuf-lite.pc @ONLY)

set(_protobuf_libraries libprotobuf-lite libprotobuf)
if (protobuf_BUILD_LIBPROTOC)
    list(APPEND _protobuf_libraries libprotoc)
endif (protobuf_BUILD_LIBPROTOC)

foreach(_library ${_protobuf_libraries})
  set_property(TARGET ${_library}
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    $<BUILD_INTERFACE:${protobuf_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  if (UNIX AND NOT APPLE)
    set_property(TARGET ${_library}
      PROPERTY INSTALL_RPATH "$ORIGIN")
  elseif (APPLE)
    set_property(TARGET ${_library}
      PROPERTY INSTALL_RPATH "@loader_path")
  endif()
  install(TARGETS ${_library} EXPORT protobuf-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${_library}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${_library}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${_library})
endforeach()

if (protobuf_BUILD_PROTOC_BINARIES)
  install(TARGETS protoc EXPORT protobuf-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT protoc
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT protoc)
  if (UNIX AND NOT APPLE)
    set_property(TARGET protoc
      PROPERTY INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
  elseif (APPLE)
    set_property(TARGET protoc
      PROPERTY INSTALL_RPATH "@loader_path/../lib")
  endif()
endif (protobuf_BUILD_PROTOC_BINARIES)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/protobuf.pc ${CMAKE_CURRENT_BINARY_DIR}/protobuf-lite.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)
set(protobuf_HEADERS
  ${libprotobuf_hdrs}
  ${libprotoc_hdrs}
  ${wkt_protos_files}
  ${cpp_features_proto_proto_srcs}
  ${descriptor_proto_proto_srcs}
  ${plugin_proto_proto_srcs}
  ${java_features_proto_proto_srcs}
)
foreach(_header ${protobuf_HEADERS})
  string(FIND ${_header} "${protobuf_SOURCE_DIR}/src" _find_src)
  string(FIND ${_header} "${protobuf_SOURCE_DIR}" _find_nosrc)
  if (_find_src GREATER -1)
    set(_from_dir "${protobuf_SOURCE_DIR}/src")
  elseif (_find_nosrc GREATER -1)
    set(_from_dir "${protobuf_SOURCE_DIR}")
  endif()
  string(REPLACE "${_from_dir}" "" _header ${_header})
  get_filename_component(_extract_from "${_from_dir}/${_header}" ABSOLUTE)
  get_filename_component(_extract_name ${_header} NAME)
  get_filename_component(_extract_to "${CMAKE_INSTALL_INCLUDEDIR}/${_header}" DIRECTORY)
  install(FILES "${_extract_from}"
    DESTINATION "${_extract_to}"
    COMPONENT protobuf-headers
    RENAME "${_extract_name}")
endforeach()

# Install configuration
set(_install_cmakedir_desc "Directory relative to CMAKE_INSTALL to install the cmake configuration files")
set(_build_cmakedir_desc "Directory relative to CMAKE_CURRENT_BINARY_DIR for cmake configuration files")
set(_exampledir_desc "Directory relative to CMAKE_INSTALL_DATA to install examples")
set(_protobuf_subdir_desc "Subdirectory in which to install cmake configuration files")
if(NOT MSVC)
  set(protobuf_CMAKE_SUBDIR "cmake/protobuf" CACHE STRING "${_protobuf_subdir_desc}")
  set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/${protobuf_CMAKE_SUBDIR}" CACHE STRING "${_install_cmakedir_desc}")
  set(CMAKE_INSTALL_EXAMPLEDIR "${CMAKE_INSTALL_DATADIR}/protobuf/examples" CACHE STRING "${_exampledir_desc}")
else()
  set(protobuf_CMAKE_SUBDIR "cmake" CACHE STRING "${_protobuf_subdir_desc}")
  set(CMAKE_INSTALL_CMAKEDIR "cmake" CACHE STRING "${_cmakedir_desc}")
  set(CMAKE_INSTALL_EXAMPLEDIR "examples" CACHE STRING "${_exampledir_desc}")
endif()
set(CMAKE_BUILD_CMAKEDIR "${CMAKE_CURRENT_BINARY_DIR}/${protobuf_CMAKE_SUBDIR}" CACHE STRING "${_build_cmakedir_desc}")
mark_as_advanced(protobuf_CMAKE_SUBDIR)
mark_as_advanced(CMAKE_BUILD_CMAKEDIR)
mark_as_advanced(CMAKE_INSTALL_CMAKEDIR)
mark_as_advanced(CMAKE_INSTALL_EXAMPLEDIR)

configure_file(${protobuf_SOURCE_DIR}/cmake/protobuf-config.cmake.in
  ${CMAKE_BUILD_CMAKEDIR}/protobuf-config.cmake @ONLY)
configure_file(${protobuf_SOURCE_DIR}/cmake/protobuf-config-version.cmake.in
  ${CMAKE_BUILD_CMAKEDIR}/protobuf-config-version.cmake @ONLY)
configure_file(${protobuf_SOURCE_DIR}/cmake/protobuf-module.cmake.in
  ${CMAKE_BUILD_CMAKEDIR}/protobuf-module.cmake @ONLY)
configure_file(${protobuf_SOURCE_DIR}/cmake/protobuf-options.cmake
  ${CMAKE_BUILD_CMAKEDIR}/protobuf-options.cmake @ONLY)
configure_file(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake
  ${CMAKE_BUILD_CMAKEDIR}/protobuf-generate.cmake @ONLY)

# Allows the build directory to be used as a find directory.

install(EXPORT protobuf-targets
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  NAMESPACE protobuf::
  COMPONENT protobuf-export
)

install(DIRECTORY ${CMAKE_BUILD_CMAKEDIR}/
  DESTINATION "${CMAKE_INSTALL_CMAKEDIR}"
  COMPONENT protobuf-export
  PATTERN protobuf-targets.cmake EXCLUDE
)

option(protobuf_INSTALL_EXAMPLES "Install the examples folder" OFF)
if(protobuf_INSTALL_EXAMPLES)
  install(DIRECTORY examples/
    DESTINATION "${CMAKE_INSTALL_EXAMPLEDIR}"
    COMPONENT protobuf-examples)
endif()

if (protobuf_INSTALL_TESTS)
  install(TARGETS gmock EXPORT protobuf-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
