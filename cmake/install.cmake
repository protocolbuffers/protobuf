include(GNUInstallDirs)

foreach(_target IN LISTS protobuf_ABSL_USED_TARGETS)
  # shared abseil on windows breaks the absl::foo -> absl_foo replacement logic -
  # preempt this by a more specific replace (harmless if it doesn't apply); see GH-15883
  string(REPLACE "absl::abseil_dll" "abseil_dll" _modified_target ${_target})
  string(REPLACE :: _ _modified_target ${_modified_target})
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

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  # attach debug postfix only in debug mode
  set(protobuf_LIBRARY_POSTFIX ${protobuf_DEBUG_POSTFIX})
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/protobuf.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/protobuf.pc @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/protobuf-lite.pc.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/protobuf-lite.pc @ONLY)
if (protobuf_BUILD_LIBUPB)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/upb.pc.cmake
                ${CMAKE_CURRENT_BINARY_DIR}/upb.pc @ONLY)
endif ()

set(_protobuf_libraries libprotobuf-lite libprotobuf)
if (protobuf_BUILD_LIBPROTOC)
    list(APPEND _protobuf_libraries libprotoc)
endif (protobuf_BUILD_LIBPROTOC)
if (protobuf_BUILD_LIBUPB)
  list(APPEND _protobuf_libraries libupb)
endif ()

foreach(_library ${_protobuf_libraries})
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
  set(_protobuf_binaries protoc)
  install(TARGETS protoc EXPORT protobuf-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT protoc
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT protoc)
  if (protobuf_BUILD_LIBUPB)
    foreach (generator upb upbdefs upb_minitable)
      list(APPEND _protobuf_binaries protoc-gen-${generator})
      install(TARGETS protoc-gen-${generator} EXPORT protobuf-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT upb-generators
        BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT upb-generators)
    endforeach ()
  endif ()
  foreach (binary IN LISTS _protobuf_binaries)
    if (UNIX AND NOT APPLE)
      set_property(TARGET ${binary}
        PROPERTY INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    elseif (APPLE)
      set_property(TARGET ${binary}
        PROPERTY INSTALL_RPATH "@loader_path/../lib")
    endif ()
  endforeach ()
endif (protobuf_BUILD_PROTOC_BINARIES)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/protobuf.pc ${CMAKE_CURRENT_BINARY_DIR}/protobuf-lite.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
if (protobuf_BUILD_LIBUPB)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/upb.pc DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
endif ()

include(${protobuf_SOURCE_DIR}/src/file_lists.cmake)
set(protobuf_HEADERS
  ${libprotobuf_hdrs}
  ${libprotoc_public_hdrs}
  ${wkt_protos_files}
  ${cpp_features_proto_proto_srcs}
  ${descriptor_proto_proto_srcs}
  ${plugin_proto_proto_srcs}
  ${java_features_proto_proto_srcs}
  ${go_features_proto_proto_srcs}
)
if (protobuf_BUILD_LIBUPB)
  list(APPEND protobuf_HEADERS ${libupb_hdrs})
  # Manually install the bootstrap headers
  install(
    FILES
      ${protobuf_SOURCE_DIR}/upb/reflection/cmake/google/protobuf/descriptor.upb.h
      ${protobuf_SOURCE_DIR}/upb/reflection/cmake/google/protobuf/descriptor.upb_minitable.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/google/protobuf
    COMPONENT protobuf-headers
  )
endif ()
set(protobuf_STRIP_PREFIXES
  "/src"
  "/java/core/src/main/resources"
  "/go"
  "/"
)
foreach(_header ${protobuf_HEADERS})
  foreach(_strip_prefix ${protobuf_STRIP_PREFIXES})
    string(FIND ${_header} "${protobuf_SOURCE_DIR}${_strip_prefix}" _find_src)
    if(_find_src GREATER -1)
      set(_from_dir "${protobuf_SOURCE_DIR}${_strip_prefix}")
      break()
    endif()
  endforeach()

  # Escape _from_dir for regex special characters in the directory name.
  string(REGEX REPLACE "([$^.[|*+?()]|])" "\\\\\\1" _from_dir_regexp "${_from_dir}")
  # On some platforms `_form_dir` ends up being just "protobuf", which can
  # easily match multiple times in our paths.  We force it to only replace
  # prefixes to avoid this case.
  string(REGEX REPLACE "^${_from_dir_regexp}" "" _header ${_header})
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
set(protobuf_CMAKE_SUBDIR "cmake/protobuf" CACHE STRING "${_protobuf_subdir_desc}")
set(CMAKE_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/${protobuf_CMAKE_SUBDIR}" CACHE STRING "${_install_cmakedir_desc}")
set(CMAKE_INSTALL_EXAMPLEDIR "${CMAKE_INSTALL_DATADIR}/protobuf/examples" CACHE STRING "${_exampledir_desc}")
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
