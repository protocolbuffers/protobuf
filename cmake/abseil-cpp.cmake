# Setup our dependency on Abseil.

if(protobuf_BUILD_TESTS)
  # Tell Abseil to build test-only helpers.
  set(ABSL_BUILD_TEST_HELPERS ON)

  # We depend on googletest too, so just tell Abseil to use the same one we've
  # already setup.
  set(ABSL_USE_EXTERNAL_GOOGLETEST ON)
  set(ABSL_FIND_GOOGLETEST OFF)
endif()

if (NOT TARGET absl::strings)
  if (NOT protobuf_FORCE_FETCH_DEPENDENCIES)
    # Use "CONFIG" as there is no built-in cmake module for absl.
    find_package(absl CONFIG)
  endif()

  # Fallback to fetching Abseil from github if it's not found locally.
  if (NOT absl_FOUND AND NOT protobuf_LOCAL_DEPENDENCIES_ONLY)
    include(${protobuf_SOURCE_DIR}/cmake/dependencies.cmake)
    message(STATUS "Fallback to downloading Abseil ${abseil-cpp-version} from GitHub")

    include(FetchContent)
    FetchContent_Declare(
      absl
      GIT_REPOSITORY "https://github.com/abseil/abseil-cpp.git"
      GIT_TAG "${abseil-cpp-version}"
    )
    if (protobuf_INSTALL)
      # When protobuf_INSTALL is enabled and Abseil will be built as a module,
      # Abseil will be installed along with protobuf for convenience.
      set(ABSL_ENABLE_INSTALL ON)
    endif()
    FetchContent_MakeAvailable(absl)
  endif()
endif()

if (NOT TARGET absl::strings)
  message(FATAL_ERROR "Cannot find abseil-cpp dependency that's needed to build protobuf.\n")
endif()

set(_protobuf_FIND_ABSL "if(NOT TARGET absl::strings)\n  find_package(absl CONFIG)\nendif()")

if (BUILD_SHARED_LIBS AND MSVC)
  # On MSVC Abseil is bundled into a single DLL.
  # This condition is necessary as of abseil 20230125.3 when abseil is consumed
  # via add_subdirectory, the abseil_dll target is named abseil_dll, while if
  # abseil is consumed via find_package, the target is called absl::abseil_dll
  # Once https://github.com/abseil/abseil-cpp/pull/1466 is merged and released
  # in the minimum version of abseil required by protobuf, it is possible to
  # always link absl::abseil_dll and absl::abseil_test_dll and remove the if
  set(protobuf_ABSL_USED_TARGETS absl::abseil_dll)
  set(protobuf_ABSL_USED_TEST_TARGETS absl::abseil_test_dll)
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
    absl::log_globals
    absl::log_severity
    absl::memory
    absl::node_hash_map
    absl::node_hash_set
    absl::optional
    absl::random_distributions
    absl::random_random
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
