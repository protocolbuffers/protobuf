include(GNUInstallDirs)

foreach(_library
  libprotobuf-lite
  libprotobuf
  libprotoc)
  set_property(TARGET ${_library}
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES $<INSTALL_INTERFACE:include>)
  install(TARGETS ${_library} EXPORT protobuf-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${_library}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${_library}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${_library})
endforeach()

install(TARGETS protoc EXPORT protobuf-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT protoc)

if(MSVC)
  file(STRINGS extract_includes.bat.in _extract_strings
    REGEX "^copy")
  foreach(_extract_string ${_extract_strings})
    string(REPLACE "copy \${PROTOBUF_SOURCE_WIN32_PATH}\\" ""
      _extract_string ${_extract_string})
    string(REPLACE "\\" "/" _extract_string ${_extract_string})
    string(REGEX MATCH "^[^ ]+"
      _extract_from ${_extract_string})
    string(REGEX REPLACE "^${_extract_from} ([^$]+)" "\\1"
      _extract_to ${_extract_string})
    get_filename_component(_extract_from "${protobuf_SOURCE_DIR}/${_extract_from}" ABSOLUTE)
    get_filename_component(_extract_name ${_extract_to} NAME)
    get_filename_component(_extract_to ${_extract_to} PATH)
    if(EXISTS "${_extract_from}")
      install(FILES "${_extract_from}"
        DESTINATION "${_extract_to}"
        COMPONENT protobuf-headers
        RENAME "${_extract_name}")
    else()
      message(AUTHOR_WARNING "The file \"${_extract_from}\" is listed in "
        "\"${protobuf_SOURCE_DIR}/cmake/extract_includes.bat.in\" "
        "but there not exists. The file will not be installed.")
    endif()
  endforeach()
endif()

install(EXPORT protobuf-targets
  DESTINATION "lib/cmake/protobuf"
  COMPONENT protobuf-export)

configure_file(protobuf-config.cmake.in
  protobuf-config.cmake @ONLY)
configure_file(protobuf-config-version.cmake.in
  protobuf-config-version.cmake @ONLY)

install(FILES
  "${protobuf_BINARY_DIR}/protobuf-config.cmake"
  "${protobuf_BINARY_DIR}/protobuf-config-version.cmake"
  DESTINATION "lib/cmake/protobuf"
  COMPONENT protobuf-export)
