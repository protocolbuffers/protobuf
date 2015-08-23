include(GNUInstallDirs)

foreach(_library
  libprotobuf-lite
  libprotobuf
  libprotoc)
  set_property(TARGET ${_library}
    PROPERTY INTERFACE_INCLUDE_DIRECTORIES
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
  install(TARGETS ${_library} EXPORT protobuf-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${_library}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${_library}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${_library})
endforeach()

install(TARGETS protoc EXPORT protobuf-targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT protoc)

if(TRUE)
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
    string(REPLACE "include/" "${CMAKE_INSTALL_INCLUDEDIR}/"
      _extract_to "${_extract_to}")
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

# Internal function for parsing auto tools scripts
function(_protobuf_auto_list FILE_NAME VARIABLE)
  file(STRINGS ${FILE_NAME} _strings)
  set(_list)
  foreach(_string ${_strings})
    set(_found)
    string(REGEX MATCH "^[ \t]*${VARIABLE}[ \t]*=[ \t]*" _found "${_string}")
    if(_found)
      string(LENGTH "${_found}" _length)
      string(SUBSTRING "${_string}" ${_length} -1 _draft_list)
      foreach(_item ${_draft_list})
        string(STRIP "${_item}" _item)
        list(APPEND _list "${_item}")
      endforeach()
    endif()
  endforeach()
  set(${VARIABLE} ${_list} PARENT_SCOPE)
endfunction()

# Install well-known type proto files
_protobuf_auto_list("../src/Makefile.am" nobase_dist_proto_DATA)
foreach(_file ${nobase_dist_proto_DATA})
  get_filename_component(_file_from "../src/${_file}" ABSOLUTE)
  get_filename_component(_file_name ${_file} NAME)
  get_filename_component(_file_path ${_file} PATH)
  if(EXISTS "${_file_from}")
    install(FILES "${_file_from}"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${_file_path}"
      COMPONENT protobuf-protos
      RENAME "${_file_name}")
  else()
    message(AUTHOR_WARNING "The file \"${_file_from}\" is listed in "
      "\"${protobuf_SOURCE_DIR}/../src/Makefile.am\" as nobase_dist_proto_DATA "
      "but there not exists. The file will not be installed.")
  endif()
endforeach()

# Export configuration

install(EXPORT protobuf-targets
  DESTINATION "lib/cmake/protobuf"
  COMPONENT protobuf-export)

configure_file(protobuf-config.cmake.in
  protobuf-config.cmake @ONLY)
configure_file(protobuf-config-version.cmake.in
  protobuf-config-version.cmake @ONLY)
configure_file(protobuf-module.cmake.in
  protobuf-module.cmake @ONLY)

install(FILES
  "${protobuf_BINARY_DIR}/protobuf-config.cmake"
  "${protobuf_BINARY_DIR}/protobuf-config-version.cmake"
  "${protobuf_BINARY_DIR}/protobuf-module.cmake"
  DESTINATION "lib/cmake/protobuf"
  COMPONENT protobuf-export)
