option(protobuf_USE_EXTERNAL_GTEST "Use external Google Test (i.e. not the one in third_party/googletest)" OFF)

if (TARGET GTest::gmock)
  # GTest is already present.
elseif (protobuf_USE_EXTERNAL_GTEST)
  find_package(GTest REQUIRED CONFIG)
else ()
  if (NOT protobuf_FETCH_DEPENDENCIES)
    message(FATAL_ERROR
            "Cannot find local googletest directory that's needed to "
            "build tests.\n"
            "If instead you want to skip tests, run cmake with:\n"
            "  cmake -Dprotobuf_BUILD_TESTS=OFF\n")
  endif()
  include(${protobuf_SOURCE_DIR}/cmake/dependencies.cmake)
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
