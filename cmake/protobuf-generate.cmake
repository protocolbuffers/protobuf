function(protobuf_generate)
  include(CMakeParseArguments)

  set(_options APPEND_PATH)
  set(_singleargs LANGUAGE OUT_VAR EXPORT_MACRO PROTOC_OUT_DIR PLUGIN PLUGIN_OPTIONS DEPENDENCIES)
  if(COMMAND target_sources)
    list(APPEND _singleargs TARGET)
  endif()
  set(_multiargs PROTOS IMPORT_DIRS GENERATE_EXTENSIONS PROTOC_OPTIONS)

  cmake_parse_arguments(protobuf_generate "${_options}" "${_singleargs}" "${_multiargs}" "${ARGN}")

  if(NOT protobuf_generate_PROTOS AND NOT protobuf_generate_TARGET)
    message(SEND_ERROR "Error: protobuf_generate called without any targets or source files")
    return()
  endif()

  if(NOT protobuf_generate_OUT_VAR AND NOT protobuf_generate_TARGET)
    message(SEND_ERROR "Error: protobuf_generate called without a target or output variable")
    return()
  endif()

  if(NOT protobuf_generate_LANGUAGE)
    set(protobuf_generate_LANGUAGE cpp)
  endif()
  string(TOLOWER ${protobuf_generate_LANGUAGE} protobuf_generate_LANGUAGE)

  if(NOT protobuf_generate_PROTOC_OUT_DIR)
    set(protobuf_generate_PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
  endif()

  if(protobuf_generate_EXPORT_MACRO AND protobuf_generate_LANGUAGE STREQUAL cpp)
    set(_dll_export_decl "dllexport_decl=${protobuf_generate_EXPORT_MACRO}")
  endif()

  foreach(_option ${_dll_export_decl} ${protobuf_generate_PLUGIN_OPTIONS})
    # append comma - not using CMake lists and string replacement as users
    # might have semicolons in options
    if(_plugin_options)
      set( _plugin_options "${_plugin_options},")
    endif()
    set(_plugin_options "${_plugin_options}${_option}")
  endforeach()

  if(protobuf_generate_PLUGIN)
      set(_plugin "--plugin=${protobuf_generate_PLUGIN}")
  endif()

  if(NOT protobuf_generate_GENERATE_EXTENSIONS)
    if(protobuf_generate_LANGUAGE STREQUAL cpp)
      set(protobuf_generate_GENERATE_EXTENSIONS .pb.h .pb.cc)
    elseif(protobuf_generate_LANGUAGE STREQUAL python)
      set(protobuf_generate_GENERATE_EXTENSIONS _pb2.py)
    else()
      message(SEND_ERROR "Error: protobuf_generate given unknown Language ${LANGUAGE}, please provide a value for GENERATE_EXTENSIONS")
      return()
    endif()
  endif()

  if(protobuf_generate_TARGET)
    get_target_property(_source_list ${protobuf_generate_TARGET} SOURCES)
    foreach(_file ${_source_list})
      if(_file MATCHES "proto$")
        list(APPEND protobuf_generate_PROTOS ${_file})
      endif()
    endforeach()
  endif()

  if(NOT protobuf_generate_PROTOS)
    message(SEND_ERROR "Error: protobuf_generate could not find any .proto files")
    return()
  endif()

  if(protobuf_generate_APPEND_PATH)
    # Create an include path for each file specified
    foreach(_file ${protobuf_generate_PROTOS})
      get_filename_component(_abs_file ${_file} ABSOLUTE)
      get_filename_component(_abs_dir ${_abs_file} DIRECTORY)
      list(FIND _protobuf_include_path ${_abs_dir} _contains_already)
      if(${_contains_already} EQUAL -1)
          list(APPEND _protobuf_include_path -I ${_abs_dir})
      endif()
    endforeach()
  endif()

  foreach(DIR ${protobuf_generate_IMPORT_DIRS})
    get_filename_component(ABS_PATH ${DIR} ABSOLUTE)
    list(FIND _protobuf_include_path ${ABS_PATH} _contains_already)
    if(${_contains_already} EQUAL -1)
        list(APPEND _protobuf_include_path -I ${ABS_PATH})
    endif()
  endforeach()

  if(NOT _protobuf_include_path)
    set(_protobuf_include_path -I ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  set(_generated_srcs_all)
  foreach(_proto ${protobuf_generate_PROTOS})
    get_filename_component(_abs_file ${_proto} ABSOLUTE)
    get_filename_component(_abs_dir ${_abs_file} DIRECTORY)

    get_filename_component(_file_full_name ${_proto} NAME)
    string(FIND "${_file_full_name}" "." _file_last_ext_pos REVERSE)
    string(SUBSTRING "${_file_full_name}" 0 ${_file_last_ext_pos} _basename)

    set(_suitable_include_found FALSE)
    foreach(DIR ${_protobuf_include_path})
      if(NOT DIR STREQUAL "-I")
        file(RELATIVE_PATH _rel_dir ${DIR} ${_abs_dir})
        if(_rel_dir STREQUAL _abs_dir)
          # When there is no relative path from DIR to _abs_dir (e.g. due to
          # different drive letters on Windows), _rel_dir is equal to _abs_dir.
          # Therefore, DIR is not a suitable include path and must be skipped.
          continue()
        endif()
        string(FIND "${_rel_dir}" "../" _is_in_parent_folder)
        if (NOT ${_is_in_parent_folder} EQUAL 0)
          set(_suitable_include_found TRUE)
          break()
        endif()
      endif()
    endforeach()

    if(NOT _suitable_include_found)
      message(SEND_ERROR "Error: protobuf_generate could not find any correct proto include directory.")
      return()
    endif()

    set(_generated_srcs)
    foreach(_ext ${protobuf_generate_GENERATE_EXTENSIONS})
      list(APPEND _generated_srcs "${protobuf_generate_PROTOC_OUT_DIR}/${_rel_dir}/${_basename}${_ext}")
    endforeach()
    list(APPEND _generated_srcs_all ${_generated_srcs})

    set(_comment "Running ${protobuf_generate_LANGUAGE} protocol buffer compiler on ${_proto}")
    if(protobuf_generate_PROTOC_OPTIONS)
      set(_comment "${_comment}, protoc-options: ${protobuf_generate_PROTOC_OPTIONS}")
    endif()
    if(_plugin_options)
      set(_comment "${_comment}, plugin-options: ${_plugin_options}")
    endif()

    add_custom_command(
      OUTPUT ${_generated_srcs}
      COMMAND protobuf::protoc
      ARGS ${protobuf_generate_PROTOC_OPTIONS} --${protobuf_generate_LANGUAGE}_out ${_plugin_options}:${protobuf_generate_PROTOC_OUT_DIR} ${_plugin} ${_protobuf_include_path} ${_abs_file}
      DEPENDS ${_abs_file} ${protobuf_PROTOC_EXE} ${protobuf_generate_DEPENDENCIES}
      COMMENT ${_comment}
      VERBATIM )
  endforeach()

  set_source_files_properties(${_generated_srcs_all} PROPERTIES GENERATED TRUE)
  if(protobuf_generate_OUT_VAR)
    set(${protobuf_generate_OUT_VAR} ${_generated_srcs_all} PARENT_SCOPE)
  endif()
  if(protobuf_generate_TARGET)
    target_sources(${protobuf_generate_TARGET} PRIVATE ${_generated_srcs_all})
  endif()

endfunction()
