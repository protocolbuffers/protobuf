# Minimum CMake required
cmake_minimum_required(VERSION 2.8)

# Project
project(protobuf C CXX)

# Options
option(BUILD_TESTING "Build tests" ON)
option(BUILD_SHARED_LIBS "Build Shared Libraries" OFF)
if (MSVC)
  option(ZLIB "Build with zlib support" OFF)
endif (MSVC)

# Path to main configure script
set(protobuf_CONFIGURE_SCRIPT "../configure.ac")

# Parse version from configure script
file(STRINGS "${protobuf_CONFIGURE_SCRIPT}" protobuf_VERSION_LINE
  LIMIT_COUNT 1
  REGEX "^AC_INIT")
# Replace special characters
string(REPLACE "(" "_" protobuf_VERSION_LINE ${protobuf_VERSION_LINE})
string(REPLACE ")" "_" protobuf_VERSION_LINE ${protobuf_VERSION_LINE})
string(REPLACE "[" "_" protobuf_VERSION_LINE ${protobuf_VERSION_LINE})
string(REPLACE "]" "_" protobuf_VERSION_LINE ${protobuf_VERSION_LINE})
# Parse version string
string(REGEX REPLACE "^AC_INIT__Protocol Buffers_,_([^_]+).*$" "\\1"
    protobuf_VERSION_STRING "${protobuf_VERSION_LINE}")
# Parse version tweaks
string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" "\\1"
  protobuf_VERSION_MAJOR "${protobuf_VERSION_STRING}")
string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" "\\2"
  protobuf_VERSION_MINOR "${protobuf_VERSION_STRING}")
string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\.([0-9]+).*$" "\\3"
  protobuf_VERSION_PATCH "${protobuf_VERSION_STRING}")
# Package version
set(protobuf_VERSION
  "${protobuf_VERSION_MAJOR}.${protobuf_VERSION_MINOR}.${protobuf_VERSION_PATCH}")

add_definitions(-DGOOGLE_PROTOBUF_CMAKE_BUILD)

find_package(Threads REQUIRED)
if (CMAKE_USE_PTHREADS_INIT)
  add_definitions(-DHAVE_PTHREAD)
endif (CMAKE_USE_PTHREADS_INIT)

if (MSVC)
  if (ZLIB)
    set(HAVE_ZLIB 1)
    find_path(ZLIB_INCLUDE_DIRECTORIES zlib.h ${protobuf_SOURCE_DIR})
    find_library(ZLIB_LIBRARIES zdll ${protobuf_SOURCE_DIR})
  else (ZLIB)
    set(HAVE_ZLIB 0)
  endif (ZLIB)
else (MSVC)
  find_package(ZLIB)
  if (ZLIB_FOUND)
    set(HAVE_ZLIB 1)
  else (ZLIB_FOUND)
    set(HAVE_ZLIB 0)
    # Explicitly set these to empty (override NOT_FOUND) so cmake doesn't
    # complain when we use them later.
    set(ZLIB_INCLUDE_DIRECTORIES)
    set(ZLIB_LIBRARIES)
  endif (ZLIB_FOUND)
endif (MSVC)

if (HAVE_ZLIB)
  add_definitions(-DHAVE_ZLIB)
endif (HAVE_ZLIB)

if (MSVC)
  if (BUILD_SHARED_LIBS)
    add_definitions(-DPROTOBUF_USE_DLLS)
  else (BUILD_SHARED_LIBS)
    # In case we are building static libraries, link also the runtime library statically
	# so that MSVCR*.DLL is not required at runtime.
    # https://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
    # This is achieved by replacing msvc option /MD with /MT and /MDd with /MTd
    # http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_build_my_MSVC_application_with_a_static_runtime.3F
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
  endif (BUILD_SHARED_LIBS)
  add_definitions(/wd4244 /wd4267 /wd4018 /wd4355 /wd4800 /wd4251 /wd4996 /wd4146 /wd4305)
endif (MSVC)

if (MSVC)
  string(REPLACE "/" "\\" PROTOBUF_SOURCE_WIN32_PATH ${protobuf_SOURCE_DIR})
  string(REPLACE "/" "\\" PROTOBUF_BINARY_WIN32_PATH ${protobuf_BINARY_DIR})
  configure_file(extract_includes.bat.in extract_includes.bat)
endif (MSVC)

get_filename_component(protobuf_source_dir ${protobuf_SOURCE_DIR} PATH)

include_directories(
  ${ZLIB_INCLUDE_DIRECTORIES}
  ${protobuf_BINARY_DIR}
  ${protobuf_source_dir}/src)

if (MSVC)
  # Add the "lib" prefix for generated .lib outputs.
  set(LIB_PREFIX lib)
else (MSVC)
  # When building with "make", "lib" prefix will be added automatically by
  # the build tool.
  set(LIB_PREFIX)
endif (MSVC)

include(libprotobuf-lite.cmake)
include(libprotobuf.cmake)
include(libprotoc.cmake)
include(protoc.cmake)

if (BUILD_TESTING)
  include(tests.cmake)
endif (BUILD_TESTING)

include(install.cmake)
