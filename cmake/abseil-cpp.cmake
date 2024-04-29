# Setup our dependency on Abseil.

if(protobuf_BUILD_TESTS)
  # Tell Abseil to build test-only helpers.
  set(ABSL_BUILD_TEST_HELPERS ON)

  # We depend on googletest too, so just tell Abseil to use the same one we've
  # already setup.
  set(ABSL_USE_EXTERNAL_GOOGLETEST ON)
  set(ABSL_FIND_GOOGLETEST OFF)
endif()

if(TARGET absl::strings)
  # If Abseil is included already, skip including it.
  # (https://github.com/protocolbuffers/protobuf/issues/10435)
elseif(protobuf_ABSL_PROVIDER STREQUAL "module")
  if(NOT ABSL_ROOT_DIR)
    set(ABSL_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/abseil-cpp)
  endif()
  if(EXISTS "${ABSL_ROOT_DIR}/CMakeLists.txt")
    if(protobuf_INSTALL)
      # When protobuf_INSTALL is enabled and Abseil will be built as a module,
      # Abseil will be installed along with protobuf for convenience.
      set(ABSL_ENABLE_INSTALL ON)
    endif()
    add_subdirectory(${ABSL_ROOT_DIR} third_party/abseil-cpp)
  else()
    message(WARNING "protobuf_ABSL_PROVIDER is \"module\" but ABSL_ROOT_DIR is wrong")
  endif()
  if(protobuf_INSTALL AND NOT _protobuf_INSTALL_SUPPORTED_FROM_MODULE)
    message(WARNING "protobuf_INSTALL will be forced to FALSE because protobuf_ABSL_PROVIDER is \"module\" and CMake version (${CMAKE_VERSION}) is less than 3.13.")
    set(protobuf_INSTALL FALSE)
  endif()
elseif(protobuf_ABSL_PROVIDER STREQUAL "package")
  # Use "CONFIG" as there is no built-in cmake module for absl.
  find_package(absl REQUIRED CONFIG)
endif()
set(_protobuf_FIND_ABSL "if(NOT TARGET absl::strings)\n  find_package(absl CONFIG)\nendif()")

if (BUILD_SHARED_LIBS AND MSVC)
  # On MSVC Abseil is bundled into a single DLL.
  # This condition is necessary as of abseil 20230125.3 when abseil is consumed via add_subdirectory,
  # the abseil_dll target  is named abseil_dll, while if abseil is consumed via find_package, the target
  # is called absl::abseil_dll
  # Once https://github.com/abseil/abseil-cpp/pull/1466 is merged and released in the minimum version of 
  # abseil required by protobuf, it is possible to always link absl::abseil_dll and absl::abseil_test_dll
  # and remove the if
  if(protobuf_ABSL_PROVIDER STREQUAL "package")
    set(protobuf_ABSL_USED_TARGETS absl::abseil_dll)
    set(protobuf_ABSL_USED_TEST_TARGETS absl::abseil_test_dll)
  else()
    set(protobuf_ABSL_USED_TARGETS abseil_dll)
    set(protobuf_ABSL_USED_TEST_TARGETS abseil_test_dll)
  endif()
else()
  set(protobuf_ABSL_USED_TARGETS
    absl::absl_check
    absl::absl_log
    absl::algorithm
    absl::base
    absl::bind_front
    absl::bits
    absl::btree
    absl::cleanup
    absl::cord
    absl::core_headers
    absl::debugging
    absl::die_if_null
    absl::dynamic_annotations
    absl::flags
    absl::flat_hash_map
    absl::flat_hash_set
    absl::function_ref
    absl::hash
    absl::layout
    absl::log_initialize
    absl::log_severity
    absl::memory
    absl::node_hash_map
    absl::node_hash_set
    absl::optional
    absl::span
    absl::status
    absl::statusor
    absl::strings
    absl::synchronization
    absl::time
    absl::type_traits
    absl::utility
    absl::variant
  )
  set(protobuf_ABSL_USED_TEST_TARGETS
    absl::scoped_mock_log
  )
endif ()
