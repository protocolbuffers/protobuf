option(protobuf_USE_EXTERNAL_GTEST "Use external Google Test (i.e. not the one in third_party/googletest)" OFF)

if (protobuf_USE_EXTERNAL_GTEST)
  find_package(GTest REQUIRED CONFIG)
else()
  if (NOT EXISTS "${protobuf_SOURCE_DIR}/third_party/googletest/CMakeLists.txt")
    message(FATAL_ERROR
            "Cannot find third_party/googletest directory that's needed to "
            "build tests. If you use git, make sure you have cloned submodules:\n"
            "  git submodule update --init --recursive\n"
            "If instead you want to skip tests, run cmake with:\n"
            "  cmake -Dprotobuf_BUILD_TESTS=OFF\n")
  endif()

  set(googlemock_source_dir "${protobuf_SOURCE_DIR}/third_party/googletest/googlemock")
  set(googletest_source_dir "${protobuf_SOURCE_DIR}/third_party/googletest/googletest")
  include_directories(
    ${googlemock_source_dir}
    ${googletest_source_dir}
    ${googletest_source_dir}/include
    ${googlemock_source_dir}/include
  )

  add_library(gmock ${protobuf_SHARED_OR_STATIC}
    "${googlemock_source_dir}/src/gmock-all.cc"
    "${googletest_source_dir}/src/gtest-all.cc"
  )
  if (protobuf_BUILD_SHARED_LIBS)
    set_target_properties(gmock
      PROPERTIES
        COMPILE_DEFINITIONS
          "GTEST_CREATE_SHARED_LIBRARY=1"
    )

  endif()
  if (protobuf_INSTALL)
    set(protobuf_INSTALL_TESTS ON)
  endif()

  target_link_libraries(gmock ${CMAKE_THREAD_LIBS_INIT})
  add_library(gmock_main STATIC "${googlemock_source_dir}/src/gmock_main.cc")
  target_link_libraries(gmock_main gmock)

  add_library(GTest::gmock ALIAS gmock)
  add_library(GTest::gmock_main ALIAS gmock_main)
  add_library(GTest::gtest ALIAS gmock)
  add_library(GTest::gtest_main ALIAS gmock_main)
endif()
