# Protocol Buffers - Google's data interchange format
# Copyright 2026 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""protobuf's CMake build and CMake test suite under Bazel, per configuration.

`protobuf_cmake_bazel_config()` defines, for one configuration of the
`linux-cmake` CI matrix (.github/workflows/test_cpp.yml):

*   `protobuf_cmake_bazel<suffix>`: the libraries, protoc and the upb
    generators, built and installed with cmake_bazel() (tests off).
*   `protobuf_cmake_bazel<suffix>_tests_build`: protobuf's tests (and the
    conformance suite), built with cmake_bazel() against the *installed*
    package above: `protobuf_BUILD_PROTOBUF_BINARIES=OFF` makes CMakeLists.txt
    skip the library targets and `find_package(Protobuf)` the install prefix,
    so the tests' protobuf_generate() steps run that package's protoc and upb
    plugins. This is the configuration the `linux-cmake-install` job tests.
*   One sh_test per `add_test()` entry in cmake/tests.cmake and
    cmake/conformance.cmake, run through run_cmake_test.sh the way ctest would
    (from the source tree root). Bazel caches their results: a test re-runs
    only if its binary or test data changed. `full-test` is sharded.
*   `cmake_bazel<suffix>_tests`: all of the above tests.
"""

load("@rules_shell//shell:sh_test.bzl", "sh_test")
load(":cmake_bazel.bzl", "cmake_bazel")

# Cache entries shared by every protobuf CMake build. CMAKE_BUILD_TYPE is
# empty, as in the current CMake CI: a Debug build would add the "d" library
# postfix.
_COMMON_CACHE_ENTRIES = {
    "CMAKE_BUILD_TYPE": "",
    "protobuf_BUILD_EXAMPLES": "OFF",
    # Dependencies come from `deps` (CMAKE_PREFIX_PATH); never download.
    "protobuf_LOCAL_DEPENDENCIES_ONLY": "ON",
}

_TAGS = ["manual"]

# Headers for the library build's compiles, from the Bazel build of the same
# sources (see header_deps in cmake_bazel.bzl).
_LIBRARY_HEADER_DEPS = [
    "//pkg:protobuf",
    "//pkg:protobuf_lite",
    "//pkg:protoc",
    "//pkg:protoc-gen-upb",
    "//pkg:protoc-gen-upb_minitable",
    "//pkg:protoc-gen-upbdefs",
    "//pkg:upb",
]

_SRC = "@protobuf-cmake-src//"

# The source tree root, from which the tests run.
_SOURCE_TREE_ANCHOR = _SRC + ":CMakeLists.txt"

# buildifier: disable=unnamed-macro
def protobuf_cmake_bazel_config(suffix, cxx_standard, conformance, deps, test_deps):
    """Defines protobuf's CMake library build, test build and tests for one configuration.

    Args:
      suffix: Appended to every target name ("" for the default configuration).
      cxx_standard: CMAKE_CXX_STANDARD.
      conformance: Whether to build and run the conformance suite
          (protobuf_BUILD_CONFORMANCE), as the matrix entry does.
      deps: cmake_bazel() targets the library build finds (Abseil).
      test_deps: Additional cmake_bazel() targets the test build finds
          (GoogleTest, and jsoncpp if `conformance`).
    """
    lib = "protobuf_cmake_bazel" + suffix
    tests_build = lib + "_tests_build"
    cache_entries = _COMMON_CACHE_ENTRIES | {"CMAKE_CXX_STANDARD": cxx_standard}

    cmake_bazel(
        name = lib,
        srcs = [_SRC + ":lib_srcs"],
        cache_entries = cache_entries | {
            "protobuf_BUILD_CONFORMANCE": "OFF",
            "protobuf_BUILD_TESTS": "OFF",
        },
        cmake_srcs = [_SRC + ":cmake_files"],
        common_hdrs = [_SRC + ":common_hdrs"],
        header_deps = _LIBRARY_HEADER_DEPS,
        tags = _TAGS,
        visibility = ["//visibility:public"],
        deps = deps,
    )

    native.filegroup(
        name = lib + "_install_rest",
        srcs = [":" + lib],
        output_group = "install_rest",
        tags = _TAGS,
    )

    executables = [
        "tests",
        "lite-test",
        "lazily-build-dependencies-test",
        "upb-test",
        "test_plugin",
        "fake_plugin",
    ]
    if conformance:
        executables += ["conformance_test_runner", "conformance_cpp"]

    cmake_bazel(
        name = tests_build,
        testonly = True,
        # Test code, plus every non-code file (headers, .proto files, test
        # data); the library sources are not compiled here.
        srcs = [
            _SRC + ":test_code",
            _SRC + ":non_code",
        ],
        cache_entries = cache_entries | {
            "protobuf_BUILD_CONFORMANCE": "ON" if conformance else "OFF",
            "protobuf_BUILD_PROTOBUF_BINARIES": "OFF",
            "protobuf_BUILD_TESTS": "ON",
            # The tests find test_plugin/fake_plugin next to themselves instead
            # of at their absolute build-directory paths, which no longer exist
            # when the tests run (run_cmake_test.sh stages them there).
            "protobuf_ABSOLUTE_TEST_PLUGIN_PATH": "OFF",
        },
        cmake_srcs = [_SRC + ":test_cmake_files"],
        codegen_srcs = [_SRC + ":protos"],
        executables = executables,
        # Each test compile gets exactly the headers it includes, found by a
        # scan action, including those of the installed protobuf package: an
        # edit of a header (e.g. by a change merged into main) recompiles
        # only the tests that include it, not all of them. Abseil, GoogleTest
        # and jsoncpp are pinned; their whole include directories are inputs.
        header_scan_deps = [":" + lib],
        install = False,
        scan_headers = True,
        tags = _TAGS,
        deps = [":" + lib] + deps + test_deps,
    )

    for exe in executables:
        native.filegroup(
            name = "%s_bin_%s" % (tests_build, exe),
            testonly = True,
            srcs = [":" + tests_build],
            output_group = "exe_" + exe,
            tags = _TAGS,
        )

    def exe_label(exe):
        return ":%s_bin_%s" % (tests_build, exe)

    # protoc from the installed package, which `tests` runs.
    protoc = "$(rootpath :%s_install_rest)/bin/protoc" % lib

    test_data = [_SOURCE_TREE_ANCHOR, _SRC + ":test_data"]
    tests = []
    for test_name, exe, helpers, shard_count in [
        ("lite_test", "lite-test", [], None),
        ("full_test", "tests", ["test_plugin", "fake_plugin"], 8),
        ("lazily_build_dependencies_test", "lazily-build-dependencies-test", [], None),
        ("upb_test", "upb-test", [], None),
    ]:
        name = "cmake_bazel%s_%s" % (suffix, test_name)
        tests.append(":" + name)
        helper_args = ["$(rootpath %s)" % exe_label(h) for h in helpers]
        helper_data = [exe_label(h) for h in helpers]
        if exe == "tests":
            helper_args.append(protoc)
            helper_data.append(":%s_install_rest" % lib)
        sh_test(
            name = name,
            size = "large",
            srcs = ["run_cmake_test.sh"],
            args = [
                "$(rootpath %s)" % _SOURCE_TREE_ANCHOR,
                "$(rootpath %s)" % exe_label(exe),
            ] + helper_args,
            data = test_data + [exe_label(exe)] + helper_data,
            shard_count = shard_count,
            tags = _TAGS,
            target_compatible_with = ["@platforms//os:linux"],
        )

    if conformance:
        name = "cmake_bazel%s_conformance_cpp_test" % suffix
        tests.append(":" + name)

        # Mirrors `conformance_cpp_test` in cmake/conformance.cmake.
        sh_test(
            name = name,
            size = "large",
            srcs = ["run_cmake_test.sh"],
            args = [
                "$(rootpath %s)" % _SOURCE_TREE_ANCHOR,
                "$(rootpath %s)" % exe_label("conformance_test_runner"),
                "$(rootpath %s)" % exe_label("conformance_cpp"),
                "--",
                "--failure_list",
                "conformance/failure_list_cpp.txt",
                "--text_format_failure_list",
                "conformance/text_format_failure_list_cpp.txt",
                "--maximum_edition",
                "2023",
                "./conformance_cpp",
            ],
            data = test_data + [
                exe_label("conformance_test_runner"),
                exe_label("conformance_cpp"),
            ],
            tags = _TAGS,
            target_compatible_with = ["@platforms//os:linux"],
        )

    native.test_suite(
        name = "cmake_bazel%s_tests" % suffix,
        tags = _TAGS,
        tests = tests,
    )
