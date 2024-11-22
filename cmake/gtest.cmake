if (NOT TARGET GTest::gmock)
  if (NOT protobuf_FORCE_FETCH_DEPENDENCIES)
    find_package(GTest CONFIG)
  endif()

  # Fallback to fetching Googletest from github if it's not found locally.
  if (NOT GTest_FOUND AND NOT protobuf_LOCAL_DEPENDENCIES_ONLY)
    include(${protobuf_SOURCE_DIR}/cmake/dependencies.cmake)
    message(STATUS "Fallback to downloading GTest ${googletest-version} from GitHub")

    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY "https://github.com/google/googletest.git"
      GIT_TAG "v${googletest-version}"
    )
    # Due to https://github.com/google/googletest/issues/4384, we can't name this
    # GTest for use with find_package until 1.15.0.
    FetchContent_MakeAvailable(googletest)
  endif()
endif()

if (NOT TARGET GTest::gmock)
  message(FATAL_ERROR
          "Cannot find googletest dependency that's needed to build tests.\n"
          "If instead you want to skip tests, run cmake with:\n"
          "  cmake -Dprotobuf_BUILD_TESTS=OFF\n")
endif()
