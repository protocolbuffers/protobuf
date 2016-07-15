# Helpful macro used for generating source files.
# Usage:
# protobuf_generate(<output_var> [NO_DEFAULT_PATH] [GENERATED_OUT <var>]
#   [IMPORT_DIRS ...] [LANGUAGES ...])
#
# Sets up rules to generate source files for the given languages from the set
# of protobuf files in a given source list (or target in CMake 3+) and appends
# them to the list of sources. By default, it will set -I for each of the
# directories containing a protobuf file.
#
# NO_DEFAULT_PATH causes the function to only include CMAKE_CURRENT_SOURCE_DIR
#
# GENERATED_SRCS if provided will be filled with the names of the generated sources
#
# IMPORT_DIRS will set additional include directories to use with protoc
#
# Entries in LANGUAGES are the names of generators, such as cpp and python.
# Additional languages can be added by setting `protobuf_${lang}_outputs`
# to a list of strings representing the outputs of that generator.
# '<filename>' will be replaced by the basenamee of the .proto file in such strings.
# If not given, it will default to 'cpp'.
#
function(PROTOBUF_GENERATE target)
  set(_options NO_DEFAULT_PATH)
  set(_singlevalue GENERATED_SRCS)
  set(_multivalue IMPORT_DIRS LANGUAGES)

  include(CMakeParseArguments)
  cmake_parse_arguments(protobuf_generate "${_options}" "${_singlevalue}" "${_multivalue}" ${ARGN})

  if(NOT protobuf_generate_LANGUAGES)
    set(protobuf_generate_LANGUAGES cpp)
  endif()

  if(protobuf_VERBOSE)
    message(STATUS "protobuf_generate() on ${target} for languages: ${protobuf_generate_LANGUAGES}")
  endif()

  # Parse the source list and identify .proto files
  if(TARGET ${target})
    if(CMAKE_MAJOR_VERSION EQUAL 2)
      message(FATAL_ERROR "protobuf_generate() cannot operate on targets prior to CMake 3.0.0. Please pass a source list variable instead.")
    endif()

    get_target_property(_source_list ${target} SOURCES)
  elseif(NOT DEFINED ${target})
    message(FATAL_ERROR "protobuf_generate() passed undefined variable '${target}'")
  else()
    set(_source_list ${${target}})
  endif()

  foreach(_src_file ${_source_list})
    if(_src_file MATCHES "\\.proto$")
      list(APPEND _proto_list ${_src_file})
    endif()
  endforeach()

  if(NOT _proto_list)
    message(AUTHOR_WARNING "protobuf_generate() could not find any .proto files in the list given by ${target}")
    return()
  endif()

  if(protobuf_VERBOSE)
    message(STATUS "Found proto files: ${_proto_list}")
  endif()

  # Generate the list of include paths
  if(NOT protobuf_generate_NO_DEFAULT_PATH)
    # Create an include path for each file specified
    foreach(file ${_proto_list})
      get_filename_component(abs_file ${file} ABSOLUTE)
      get_filename_component(abs_path ${abs_file} PATH)
	    list(APPEND _include_paths ${abs_path})
    endforeach()
  else()
    set(_include_paths ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  foreach(import_dir ${protobuf_generate_IMPORT_DIRS})
    get_filename_component(abs_path ${import_dir} ABSOLUTE)
	  list(APPEND _include_paths ${abs_path})
  endforeach()

  if(protobuf_VERBOSE)
    message(STATUS "Using include paths: ${_include_paths}")
  endif()

  list(REMOVE_DUPLICATES _include_paths)
  string(REPLACE ";" ";-I;" _include_string ";${_include_paths}")

  # Output lists for the built-in languages
  set(protobuf_cpp_outputs "${CMAKE_CURRENT_BINARY_DIR}/<filename>.pb.cc"
                  "${CMAKE_CURRENT_BINARY_DIR}/<filename>.pb.h")
  set(protobuf_python_outputs "${CMAKE_CURRENT_BINARY_DIR}/<filename>_pb3.py")

  foreach(_proto_file ${_proto_list})
    get_filename_component(abs_file ${_proto_file} ABSOLUTE)
    get_filename_component(base_file ${_proto_file} NAME_WE)

    foreach(lang ${protobuf_generate_LANGUAGES})
      if(NOT DEFINED protobuf_${lang}_outputs)
        message(FATAL_ERROR "Unknown language ${lang}: protobuf_${lang}_outputs not defined")
      endif()

      string(REPLACE "<filename>" "${base_file}" _outputs "${protobuf_${lang}_outputs}")
      list(APPEND _generated_files ${_outputs})

      add_custom_command(
        OUTPUT ${_outputs}
        COMMAND protobuf::protoc
        ARGS --${lang}_out  ${CMAKE_CURRENT_BINARY_DIR} ${_include_string} ${abs_file}
        DEPENDS ${_proto_file} protobuf::protoc
        COMMENT "Running ${lang} protocol buffer compiler on ${_proto_file}"
        VERBATIM)
    endforeach()
  endforeach()

  if(protobuf_VERBOSE)
    message(STATUS "protobuf_generate() created ${_generated_files}")
  endif()

  set_source_files_properties(${_generated_files} PROPERTIES GENERATED TRUE)

  if(protobuf_generate_GENERATED_SRCS)
    set(${protobuf_generate_GENERATED_SRCS} ${_generated_files} PARENT_SCOPE)
  endif()

  if(TARGET ${target})
    target_sources(${target} PRIVATE ${_generated_files})
  else()
    list(APPEND ${target} ${_generated_files})
    set(${target} ${${target}} PARENT_SCOPE)
  endif()
endfunction()
