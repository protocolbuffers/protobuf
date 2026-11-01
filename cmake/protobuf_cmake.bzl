# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""protobuf's CMake build and tests as rules_foreign_cc targets, per configuration.

`protobuf_cmake()` defines one configuration of the CMake build -- the
equivalent of one entry of the `linux-cmake` matrix in
.github/workflows/test_cpp.yml -- as a set of Bazel targets:

*   `<name>`: the libraries, protoc and the upb generators, built and
    installed with CMake (`protobuf_BUILD_TESTS=OFF`). Its input is the
    `lib_srcs` view of the staged source tree (see MODULE.bazel), so test-only
    edits do not rebuild it.
*   `<name>_tests`: the test and conformance executables, built with
    `protobuf_BUILD_PROTOBUF_BINARIES=OFF` against the install prefix of
    `<name>`, which rules_foreign_cc puts on CMAKE_PREFIX_PATH -- the
    configuration the `linux-cmake-install` job exercises.
*   `<test_prefix>_<test>`: one `sh_test` per `add_test()` in cmake/tests.cmake
    and cmake/conformance.cmake, running the executables from `<name>_tests`
    through run_cmake_test.sh, and `all_<test_prefix>_tests` grouping them.

Every configuration generates with Ninja, builds against the dependencies'
own CMake-under-Bazel targets with `protobuf_LOCAL_DEPENDENCIES_ONLY=ON`, and
is tagged `manual`.
"""

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

# Cache entries shared by every CMake invocation.
#
# CMAKE_BUILD_TYPE is deliberately empty: the current CI does not set it either,
# and a Debug build would add the "d" library postfix that the install-manifest
# goldens do not expect. rules_foreign_cc treats an empty value as "do not pass".
COMMON_CACHE_ENTRIES = {
    "CMAKE_BUILD_TYPE": "",
    "CMAKE_CXX_STANDARD": "17",
}

# Compile through sccache when --//cmake:sccache is set; see cmake/BUILD.bazel.
SCCACHE_CACHE_ENTRIES = select({
    str(Label("//cmake:with_sccache")): {
        "CMAKE_CXX_COMPILER_LAUNCHER": "sccache",
        "CMAKE_C_COMPILER_LAUNCHER": "sccache",
    },
    "//conditions:default": {},
})

CMAKE_TAGS = ["manual"]

# Executables that cmake/tests.cmake and cmake/conformance.cmake build but do
# not install. postfix_script copies them into the install prefix so they
# become declared outputs of `<name>_tests`.
_TEST_EXECUTABLES = [
    "tests",
    "lite-test",
    "lazily-build-dependencies-test",
    "upb-test",
    "test_plugin",
    "fake_plugin",
    "conformance_test_runner",
    "conformance_cpp",
]

_INSTALLED_LIBS = [
    "libprotobuf",
    "libprotobuf-lite",
    "libprotoc",
    "libutf8_range",
    "libutf8_validity",
]

_SOURCE_TREE_ANCHOR = "@protobuf-cmake-src//:CMakeLists.txt"

# What the test executables read at run time (see MODULE.bazel). This, not the
# whole staged tree, is each test's runfiles, so a test's cached result stays
# valid across changes that rebuilt the executables to byte-identical files or
# that only touched files outside this view.
_TEST_DATA = "@protobuf-cmake-src//:test_data"

def protobuf_cmake(name, test_prefix, cache_entries = {}, shared = False, visibility = None):
    """Defines one configuration of protobuf's CMake build and its tests.

    Args:
      name: Name of the library build target; the test build is `<name>_tests`.
      test_prefix: Prefix of the generated `sh_test`s (`<test_prefix>_full_test`,
        ...) and of their suite (`all_<test_prefix>_tests`).
      cache_entries: CMake cache entries layered over the defaults (C++17, no
        examples, conformance in the test build), e.g. a different
        CMAKE_CXX_STANDARD.
      shared: Build shared instead of static libraries
        (`protobuf_BUILD_SHARED_LIBS=ON`); the tests then run with the install
        prefix's lib/ on LD_LIBRARY_PATH.
      visibility: Visibility of the generated targets.
    """
    tests_name = name + "_tests"
    link_entries = {"protobuf_BUILD_SHARED_LIBS": "ON" if shared else "OFF"}

    # libupb is always static (see cmake/installed_lib_*_golden.txt).
    if shared:
        out_libs = {
            "out_shared_libs": [lib + ".so" for lib in _INSTALLED_LIBS],
            "out_static_libs": ["libupb.a"],
        }
    else:
        out_libs = {
            "out_static_libs": [lib + ".a" for lib in _INSTALLED_LIBS] + ["libupb.a"],
        }

    cmake(
        name = name,
        cache_entries = COMMON_CACHE_ENTRIES | {
            "protobuf_BUILD_CONFORMANCE": "OFF",
            "protobuf_BUILD_EXAMPLES": "OFF",
            "protobuf_BUILD_TESTS": "OFF",
            # Dependencies must come from the cmake() targets in `deps` (which
            # rules_foreign_cc places on CMAKE_PREFIX_PATH); never fall back to
            # FetchContent, which would need network access.
            "protobuf_LOCAL_DEPENDENCIES_ONLY": "ON",
        } | link_entries | cache_entries | SCCACHE_CACHE_ENTRIES,
        generate_args = ["-GNinja"],
        lib_source = "@protobuf-cmake-src//:lib_srcs",
        out_binaries = [
            "protoc",
            "protoc-gen-upb",
            "protoc-gen-upb_minitable",
            "protoc-gen-upbdefs",
        ],
        out_data_dirs = [
            "lib/cmake",
            "lib/pkgconfig",
        ],
        out_data_files = ["install_manifest.txt"],
        postfix_script = "cp $$BUILD_TMPDIR/install_manifest.txt $$INSTALLDIR/",
        # Strip the sandbox path from compiled outputs so they are reproducible
        # (and so a compiler cache such as sccache can hit across runs).
        set_file_prefix_map = True,
        tags = CMAKE_TAGS,
        visibility = visibility,
        # Abseil comes from Abseil's own CMake-under-Bazel target, configured
        # and built once. Mirrors cmake/abseil-cpp.cmake.
        deps = ["@abseil-cpp//CMake:absl"],
        **out_libs
    )

    # protobuf_INSTALL is forced off when the binaries are not built, so the
    # install step installs nothing; postfix_script copies the test executables
    # into the otherwise empty install prefix to make them declared outputs.
    cmake(
        name = tests_name,
        testonly = True,
        cache_entries = COMMON_CACHE_ENTRIES | {
            "protobuf_BUILD_CONFORMANCE": "ON",
            "protobuf_BUILD_EXAMPLES": "OFF",
            "protobuf_BUILD_PROTOBUF_BINARIES": "OFF",
            "protobuf_BUILD_TESTS": "ON",
            # Make the `tests` binary look for test_plugin/fake_plugin next to
            # itself instead of at their absolute build-directory paths, which
            # no longer exist when the sh_tests run (run_cmake_test.sh stages
            # the helpers as siblings).
            "protobuf_ABSOLUTE_TEST_PLUGIN_PATH": "OFF",
            "protobuf_LOCAL_DEPENDENCIES_ONLY": "ON",
        } | link_entries | cache_entries | SCCACHE_CACHE_ENTRIES,
        generate_args = ["-GNinja"],
        lib_source = "@protobuf-cmake-src//:srcs",
        out_binaries = _TEST_EXECUTABLES,
        postfix_script = " && ".join(
            ["mkdir -p $$INSTALLDIR/bin"] + [
                "cp $$BUILD_TMPDIR/%s $$INSTALLDIR/bin/" % exe
                for exe in _TEST_EXECUTABLES
            ],
        ),
        set_file_prefix_map = True,
        tags = CMAKE_TAGS,
        visibility = visibility,
        # GoogleTest is the build Abseil's test helper libraries (which
        # protobuf's tests link against) were built with; jsoncpp is for the
        # conformance runner. Mirrors cmake/gtest.cmake and
        # cmake/conformance.cmake.
        deps = [
            ":" + name,
            Label("//cmake:jsoncpp_cmake"),
            "@abseil-cpp//CMake:absl",
            "@abseil-cpp//CMake:googletest",
        ],
    )

    # rules_foreign_cc exposes every declared output as an output group named
    # after the file, which lets tests depend on exactly the executable they
    # need.
    native.filegroup(
        name = name + "_bin_protoc",
        srcs = [":" + name],
        output_group = "protoc",
        tags = CMAKE_TAGS,
        visibility = visibility,
    )
    for exe in _TEST_EXECUTABLES:
        native.filegroup(
            name = tests_name + "_bin_" + exe,
            testonly = True,
            srcs = [":" + tests_name],
            output_group = exe,
            tags = CMAKE_TAGS,
            visibility = visibility,
        )

    # With shared libraries the executables need the install prefix's lib/ at
    # run time. `gen_dir` is rules_foreign_cc's output group for the whole
    # install prefix.
    shared_data = []
    shared_env = {}
    if shared:
        native.filegroup(
            name = name + "_prefix",
            srcs = [":" + name],
            output_group = "gen_dir",
            tags = CMAKE_TAGS,
            visibility = visibility,
        )
        shared_data = [":" + name + "_prefix"]
        shared_env = {"PROTOBUF_CMAKE_LIB_DIR": "$(rootpath :%s_prefix)/lib" % name}

    def _bin(exe):
        return ":%s_bin_%s" % (tests_name, exe)

    # `tests` (full-test) shells out to protoc and the plugin executables.
    full_test_helpers = [
        ":" + name + "_bin_protoc",
        _bin("test_plugin"),
        _bin("fake_plugin"),
    ]

    all_tests = []
    for test_name, exe, helpers in [
        ("lite_test", "lite-test", []),
        ("full_test", "tests", full_test_helpers),
        ("lazily_build_dependencies_test", "lazily-build-dependencies-test", []),
        ("upb_test", "upb-test", []),
    ]:
        target = test_prefix + "_" + test_name
        all_tests.append(":" + target)
        sh_test(
            name = target,
            size = "large",
            srcs = [Label("//cmake:run_cmake_test.sh")],
            args = [
                "$(rootpath %s)" % _SOURCE_TREE_ANCHOR,
                "$(rootpath %s)" % _bin(exe),
            ] + [
                "$(rootpath %s)" % helper
                for helper in helpers
            ],
            data = [
                _SOURCE_TREE_ANCHOR,
                _TEST_DATA,
                _bin(exe),
            ] + helpers + shared_data,
            env = shared_env,
            tags = CMAKE_TAGS,
            visibility = visibility,
        )

    # Mirrors `conformance_cpp_test` in cmake/conformance.cmake.
    conformance = test_prefix + "_conformance_cpp_test"
    all_tests.append(":" + conformance)
    sh_test(
        name = conformance,
        size = "large",
        srcs = [Label("//cmake:run_cmake_test.sh")],
        args = [
            "$(rootpath %s)" % _SOURCE_TREE_ANCHOR,
            "$(rootpath %s)" % _bin("conformance_test_runner"),
            "$(rootpath %s)" % _bin("conformance_cpp"),
            "--",
            "--failure_list",
            "conformance/failure_list_cpp.txt",
            "--text_format_failure_list",
            "conformance/text_format_failure_list_cpp.txt",
            "--maximum_edition",
            "2023",
            "./conformance_cpp",
        ],
        data = [
            _SOURCE_TREE_ANCHOR,
            _TEST_DATA,
            _bin("conformance_cpp"),
            _bin("conformance_test_runner"),
        ] + shared_data,
        env = shared_env,
        tags = CMAKE_TAGS,
        visibility = visibility,
    )

    native.test_suite(
        name = "all_" + test_prefix + "_tests",
        tags = CMAKE_TAGS,
        tests = all_tests,
        visibility = visibility,
    )
