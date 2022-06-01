# Minimum CMake required
cmake_minimum_required(VERSION 3.5)

if(protobuf_VERBOSE)
  message(STATUS "Protocol Buffers Configuring...")
endif()

# CMake policies
cmake_policy(SET CMP0022 NEW)
# On MacOS use @rpath/ for target's install name prefix path
if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif ()
# Clear VERSION variables when no VERSION is given to project()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()
# MSVC runtime library flags are selected by an abstraction.
if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

# Project
project(protobuf C CXX)

if(protobuf_DEPRECATED_CMAKE_SUBDIRECTORY_USAGE)
  if(CMAKE_PROJECT_NAME STREQUAL "protobuf")
    get_filename_component(CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)
  endif()
  get_filename_component(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
  get_filename_component(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
  get_filename_component(protobuf_SOURCE_DIR ${protobuf_SOURCE_DIR} DIRECTORY)
endif()

# Add c++11 flags
if (CYGWIN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
else()
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

# The Intel compiler isn't able to deal with noinline member functions of
# template classes defined in headers.  As such it spams the output with
#   warning #2196: routine is both "inline" and "noinline"
# This silences that warning.
if (CMAKE_CXX_COMPILER_ID MATCHES Intel)
  string(APPEND CMAKE_CXX_FLAGS " -diag-disable=2196")
endif()

# Options
option(protobuf_INSTALL "Install protobuf binaries and files" ON)
if(WITH_PROTOC)
  set(protobuf_PROTOC_EXE ${WITH_PROTOC} CACHE FILEPATH "Protocol Buffer Compiler executable" FORCE)
endif()
option(protobuf_BUILD_TESTS "Build tests" ON)
option(protobuf_BUILD_CONFORMANCE "Build conformance tests" OFF)
option(protobuf_BUILD_EXAMPLES "Build examples" OFF)
option(protobuf_BUILD_PROTOC_BINARIES "Build libprotoc and protoc compiler" ON)
option(protobuf_BUILD_LIBPROTOC "Build libprotoc" OFF)
option(protobuf_DISABLE_RTTI "Remove runtime type information in the binaries" OFF)
if (BUILD_SHARED_LIBS)
  set(protobuf_BUILD_SHARED_LIBS_DEFAULT ON)
else (BUILD_SHARED_LIBS)
  set(protobuf_BUILD_SHARED_LIBS_DEFAULT OFF)
endif (BUILD_SHARED_LIBS)
option(protobuf_BUILD_SHARED_LIBS "Build Shared Libraries" ${protobuf_BUILD_SHARED_LIBS_DEFAULT})
include(CMakeDependentOption)
cmake_dependent_option(protobuf_MSVC_STATIC_RUNTIME "Link static runtime libraries" ON
  "NOT protobuf_BUILD_SHARED_LIBS" OFF)
set(protobuf_WITH_ZLIB_DEFAULT ON)
option(protobuf_WITH_ZLIB "Build with zlib support" ${protobuf_WITH_ZLIB_DEFAULT})
set(protobuf_DEBUG_POSTFIX "d"
  CACHE STRING "Default debug postfix")
mark_as_advanced(protobuf_DEBUG_POSTFIX)
# User options
include(${protobuf_SOURCE_DIR}/cmake/protobuf-options.cmake)

# Overrides for option dependencies
if (protobuf_BUILD_PROTOC_BINARIES OR protobuf_BUILD_TESTS)
  set(protobuf_BUILD_LIBPROTOC ON)
endif ()
# Path to main configure script
set(protobuf_CONFIGURE_SCRIPT "${protobuf_SOURCE_DIR}/configure.ac")

# Parse configure script
set(protobuf_AC_INIT_REGEX
  "^AC_INIT\\(\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\],\\[([^]]+)\\]\\)$")
file(STRINGS "${protobuf_CONFIGURE_SCRIPT}" protobuf_AC_INIT_LINE
  LIMIT_COUNT 1 REGEX "^AC_INIT")
# Description
string(REGEX REPLACE        "${protobuf_AC_INIT_REGEX}" "\\1"
    protobuf_DESCRIPTION    "${protobuf_AC_INIT_LINE}")
# Version
string(REGEX REPLACE        "${protobuf_AC_INIT_REGEX}" "\\2"
    protobuf_VERSION_STRING "${protobuf_AC_INIT_LINE}")
# Contact
string(REGEX REPLACE        "${protobuf_AC_INIT_REGEX}" "\\3"
    protobuf_CONTACT        "${protobuf_AC_INIT_LINE}")
# Parse version tweaks
set(protobuf_VERSION_REGEX "^([0-9]+)\\.([0-9]+)\\.([0-9]+)([-]rc[-]|\\.)?([0-9]*)$")
string(REGEX REPLACE     "${protobuf_VERSION_REGEX}" "\\1"
  protobuf_VERSION_MAJOR "${protobuf_VERSION_STRING}")
string(REGEX REPLACE     "${protobuf_VERSION_REGEX}" "\\2"
  protobuf_VERSION_MINOR "${protobuf_VERSION_STRING}")
string(REGEX REPLACE     "${protobuf_VERSION_REGEX}" "\\3"
  protobuf_VERSION_PATCH "${protobuf_VERSION_STRING}")
string(REGEX REPLACE     "${protobuf_VERSION_REGEX}" "\\5"
  protobuf_VERSION_PRERELEASE "${protobuf_VERSION_STRING}")

message(STATUS "${protobuf_VERSION_PRERELEASE}")

# Package version
set(protobuf_VERSION
  "${protobuf_VERSION_MAJOR}.${protobuf_VERSION_MINOR}.${protobuf_VERSION_PATCH}")

if(protobuf_VERSION_PRERELEASE)
  set(protobuf_VERSION "${protobuf_VERSION}.${protobuf_VERSION_PRERELEASE}")
else()
  set(protobuf_VERSION "${protobuf_VERSION}.0")
endif()
message(STATUS "${protobuf_VERSION}")

if(protobuf_VERBOSE)
  message(STATUS "Configuration script parsing status [")
  message(STATUS "  Description : ${protobuf_DESCRIPTION}")
  message(STATUS "  Version     : ${protobuf_VERSION} (${protobuf_VERSION_STRING})")
  message(STATUS "  Contact     : ${protobuf_CONTACT}")
  message(STATUS "]")
endif()

add_definitions(-DGOOGLE_PROTOBUF_CMAKE_BUILD)

if (protobuf_DISABLE_RTTI)
  add_definitions(-DGOOGLE_PROTOBUF_NO_RTTI=1)
endif()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cmaketest.map
"{
  global:
    main;
  local:
    *;
};")
# CheckLinkerFlag module available in CMake >=3.18.
if(${CMAKE_VERSION} VERSION_GREATER 3.18 OR ${CMAKE_VERSION} VERSION_EQUAL 3.18)
  include(CheckLinkerFlag)
  check_linker_flag(CXX -Wl,--version-script=${CMAKE_CURRENT_BINARY_DIR}/cmaketest.map protobuf_HAVE_LD_VERSION_SCRIPT)
else()
  include(CheckCXXSourceCompiles)
  set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_BINARY_DIR}/cmaketest.map)
  check_cxx_source_compiles("
    int main() {
      return 0;
    }
  " protobuf_HAVE_LD_VERSION_SCRIPT)
  set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endif()
file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/cmaketest.map)

find_package(Threads REQUIRED)

set(_protobuf_FIND_ZLIB)
if (protobuf_WITH_ZLIB)
  find_package(ZLIB)
  if (ZLIB_FOUND)
    set(HAVE_ZLIB 1)
    # FindZLIB module define ZLIB_INCLUDE_DIRS variable
    # Set ZLIB_INCLUDE_DIRECTORIES for compatible
    set(ZLIB_INCLUDE_DIRECTORIES ${ZLIB_INCLUDE_DIRECTORIES} ${ZLIB_INCLUDE_DIRS})
    # Using imported target if exists
    if (TARGET ZLIB::ZLIB)
      set(ZLIB_LIBRARIES ZLIB::ZLIB)
      set(_protobuf_FIND_ZLIB "if(NOT ZLIB_FOUND)\n  find_package(ZLIB)\nendif()")
    endif (TARGET ZLIB::ZLIB)
  else (ZLIB_FOUND)
    set(HAVE_ZLIB 0)
    # Explicitly set these to empty (override NOT_FOUND) so cmake doesn't
    # complain when we use them later.
    set(ZLIB_INCLUDE_DIRECTORIES)
    set(ZLIB_LIBRARIES)
  endif (ZLIB_FOUND)
endif (protobuf_WITH_ZLIB)

if (HAVE_ZLIB)
  add_definitions(-DHAVE_ZLIB)
endif (HAVE_ZLIB)

# We need to link with libatomic on systems that do not have builtin atomics, or
# don't have builtin support for 8 byte atomics
set(protobuf_LINK_LIBATOMIC false)
if (NOT MSVC)
  include(CheckCXXSourceCompiles)
  set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
  set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} -std=c++11)
  check_cxx_source_compiles("
    #include <atomic>
    int main() {
      return std::atomic<int64_t>{};
    }
  " protobuf_HAVE_BUILTIN_ATOMICS)
  if (NOT protobuf_HAVE_BUILTIN_ATOMICS)
    set(protobuf_LINK_LIBATOMIC true)
  endif (NOT protobuf_HAVE_BUILTIN_ATOMICS)
  set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endif (NOT MSVC)

if (protobuf_BUILD_SHARED_LIBS)
  set(protobuf_SHARED_OR_STATIC "SHARED")
else (protobuf_BUILD_SHARED_LIBS)
  set(protobuf_SHARED_OR_STATIC "STATIC")
  # The CMAKE_<LANG>_FLAGS(_<BUILD_TYPE>)? is meant to be user controlled.
  # Prior to CMake 3.15, the MSVC runtime library was pushed into the same flags
  # making programmatic control difficult.  Prefer the functionality in newer
  # CMake versions when available.
  if(${CMAKE_VERSION} VERSION_GREATER 3.15 OR ${CMAKE_VERSION} VERSION_EQUAL 3.15)
    if (protobuf_MSVC_STATIC_RUNTIME)
        set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>)
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded$<$<CONFIG:Debug>:Debug>DLL)
    endif()
  else()
    # In case we are building static libraries, link also the runtime library statically
    # so that MSVCR*.DLL is not required at runtime.
    # https://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
    # This is achieved by replacing msvc option /MD with /MT and /MDd with /MTd
    # http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime.3F
    if (MSVC AND protobuf_MSVC_STATIC_RUNTIME)
      foreach(flag_var
          CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
          CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
          string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif(${flag_var} MATCHES "/MD")
      endforeach(flag_var)
    endif (MSVC AND protobuf_MSVC_STATIC_RUNTIME)
  endif()
endif (protobuf_BUILD_SHARED_LIBS)

if (MSVC)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Build with multiple processes
    add_definitions(/MP)
  endif()
  # Set source file and execution character sets to UTF-8
  add_definitions(/utf-8)
  # MSVC warning suppressions
  add_definitions(
    /wd4065 # switch statement contains 'default' but no 'case' labels
    /wd4244 # 'conversion' conversion from 'type1' to 'type2', possible loss of data
    /wd4251 # 'identifier' : class 'type' needs to have dll-interface to be used by clients of class 'type2'
    /wd4267 # 'var' : conversion from 'size_t' to 'type', possible loss of data
    /wd4305 # 'identifier' : truncation from 'type1' to 'type2'
    /wd4307 # 'operator' : integral constant overflow
    /wd4309 # 'conversion' : truncation of constant value
    /wd4334 # 'operator' : result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    /wd4355 # 'this' : used in base member initializer list
    /wd4506 # no definition for inline function 'function'
    /wd4800 # 'type' : forcing value to bool 'true' or 'false' (performance warning)
    /wd4996 # The compiler encountered a deprecated declaration.
  )
  # Allow big object
  add_definitions(/bigobj)
  string(REPLACE "/" "\\" PROTOBUF_SOURCE_WIN32_PATH ${protobuf_SOURCE_DIR})
  string(REPLACE "/" "\\" PROTOBUF_BINARY_WIN32_PATH ${protobuf_BINARY_DIR})
  string(REPLACE "." ","  protobuf_RC_FILEVERSION "${protobuf_VERSION}")
  configure_file(${protobuf_SOURCE_DIR}/cmake/extract_includes.bat.in extract_includes.bat)

  # Suppress linker warnings about files with no symbols defined.
  set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4221")

  if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # Configure Resource Compiler
    enable_language(RC)
    # use English language (0x409) in resource compiler
    set(rc_flags "/l0x409")
    # fix rc.exe invocations because of usage of add_definitions()
    set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> ${rc_flags} <DEFINES> /fo<OBJECT> <SOURCE>")
  endif()

  # Generate the version.rc file used elsewhere.
  configure_file(${protobuf_SOURCE_DIR}/cmake/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
  set(protobuf_version_rc_file ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

  # Add the "lib" prefix for generated .lib outputs.
  set(LIB_PREFIX lib)
else (MSVC)
  # No version.rc file.
  set(protobuf_version_rc_file)

  # When building with "make", "lib" prefix will be added automatically by
  # the build tool.
  set(LIB_PREFIX)
endif (MSVC)

include_directories(
  ${ZLIB_INCLUDE_DIRECTORIES}
  ${protobuf_BINARY_DIR}
  ${protobuf_SOURCE_DIR}/src)

if (protobuf_UNICODE)
  add_definitions(-DUNICODE -D_UNICODE)
endif (protobuf_UNICODE)

include(${protobuf_SOURCE_DIR}/cmake/libprotobuf-lite.cmake)
include(${protobuf_SOURCE_DIR}/cmake/libprotobuf.cmake)
if (protobuf_BUILD_LIBPROTOC)
  include(${protobuf_SOURCE_DIR}/cmake/libprotoc.cmake)
endif (protobuf_BUILD_LIBPROTOC)
if (protobuf_BUILD_PROTOC_BINARIES)
  include(${protobuf_SOURCE_DIR}/cmake/protoc.cmake)
  if (NOT DEFINED protobuf_PROTOC_EXE)
    set(protobuf_PROTOC_EXE protoc)
  endif (NOT DEFINED protobuf_PROTOC_EXE)
endif (protobuf_BUILD_PROTOC_BINARIES)

# Ensure we have a protoc executable if we need one
if (protobuf_BUILD_TESTS OR protobuf_BUILD_CONFORMANCE OR protobuf_BUILD_EXAMPLES)
  if (NOT DEFINED protobuf_PROTOC_EXE)
    find_program(protobuf_PROTOC_EXE protoc)
    if (NOT protobuf_PROTOC_EXE)
      message(FATAL "Build requires 'protoc' but binary not found and not building protoc.")
    endif ()
  endif ()
  if(protobuf_VERBOSE)
    message(STATUS "Using protoc : ${protobuf_PROTOC_EXE}")
  endif(protobuf_VERBOSE)
endif ()

if (protobuf_BUILD_TESTS)
  enable_testing()
  include(${protobuf_SOURCE_DIR}/cmake/tests.cmake)
endif (protobuf_BUILD_TESTS)

if (protobuf_BUILD_CONFORMANCE)
  include(${protobuf_SOURCE_DIR}/cmake/conformance.cmake)
endif (protobuf_BUILD_CONFORMANCE)

if (protobuf_INSTALL)
  include(${protobuf_SOURCE_DIR}/cmake/install.cmake)
endif (protobuf_INSTALL)

if (protobuf_BUILD_EXAMPLES)
  include(${protobuf_SOURCE_DIR}/cmake/examples.cmake)
endif (protobuf_BUILD_EXAMPLES)

if(protobuf_VERBOSE)
  message(STATUS "Protocol Buffers Configuring done")
endif(protobuf_VERBOSE)
