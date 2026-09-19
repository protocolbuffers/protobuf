# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Starlark macros for the gtest-based Protobuf conformance tests.

A conformance *suite* (e.g. `binary`) is a `cc_library` of gtest-based tests
that drive a testee through the conformance protocol; see
`conformance_suite()`.  A conformance *test* runs every suite against one
testee binary, one `cc_test` per suite, with that testee's failure lists; see
`conformance_test()` and `failure_lists()`.

Typical use, in the package holding the testee:

    failure_lists(name = "failure_lists")

    conformance_test(
        name = "cpp",
        testee = ":conformance_cpp",
        failure_lists = ":failure_lists",
    )

This creates `:cpp_binary_test` (and one `:cpp_<suite>_test` per further
suite), plus a `:cpp` test_suite running all of them.  The expected failures
for `:cpp_binary_test` live in `failure_lists/cpp_binary.txt`; when that file
doesn't exist, the testee is expected to pass everything.  Either way

    blaze run :cpp_binary_test -- --fix

creates or rewrites the file to match the observed results.

conformance_test() supersedes the macro of the same name in defs.bzl, which
drives the legacy conformance_test_runner and stays until the language
runtimes have migrated to these suites.
TODO: b/563657875 - retire defs.bzl once they have.

PLEASE DO NOT DEPEND ON THE CONTENTS OF THIS FILE, IT IS UNSTABLE.
"""

load("@rules_cc//cc:cc_test.bzl", "cc_test")
load("@rules_cc//cc:defs.bzl", "cc_library")

# The conformance suites, i.e. the valid `name`s of conformance_suite().
# conformance_test() creates one test per suite, and failure lists are named
# after them (see failure_lists()).
# TODO: b/410122337 - add "text" once that suite is migrated.
# TODO: b/410122158 - add "json" once that suite is migrated.
_SUITES = ["binary"]

# The gtest main that turns a suite library into a conformance test binary, and
# the testee it links by default: --testee_binary run as a subprocess (see
# testee_runner.h for how the testee is chosen at link time).
_TEST_ENVIRONMENT_MAIN = Label("//conformance:test_environment_main")
_FORKED_TESTEE = Label("//conformance:forked_testee")

def _suite_library(suite):
    """Returns the label of the cc_library created by conformance_suite(suite)."""
    return Label("//conformance:%s_conformance_tests" % suite)

def _check_suite(suite):
    if suite not in _SUITES:
        fail("Unknown conformance suite %r; the known suites are %s." %
             (suite, ", ".join(_SUITES)))

def conformance_suite(name, srcs, deps = [], **kwargs):
    """Defines a conformance suite: a library of gtest-based conformance tests.

    Creates `cc_library(name = name + "_conformance_tests", alwayslink = 1,
    testonly = True, ...)`; no target named `name` itself is created, so
    depend on `:<name>_conformance_tests`.  Linking it into a binary that uses
    `:test_environment_main` (which conformance_test() does) registers every
    test in `srcs`; the library itself must not depend on any gtest main.

    Must be used in //conformance, which is where
    conformance_test() looks the suite libraries up.

    Args:
      name: The suite's name, one of the known suites (`binary`, ...).
      srcs: The test sources.
      deps: Their dependencies, e.g. `:test_environment` and `:matchers`.
      **kwargs: Passed through to the cc_library (e.g. `visibility`).
        `testonly` and `alwayslink` are set by the macro and can't be given.
    """
    _check_suite(name)
    library = _suite_library(name)
    if native.package_relative_label(":" + library.name) != library:
        fail("conformance_suite(%r) must be used in package //%s, not //%s." %
             (name, library.package, native.package_name()))
    for attr in ("testonly", "alwayslink"):
        if attr in kwargs:
            fail("conformance_suite(%r): %s is set by the macro and can't be overridden." %
                 (name, attr))
    cc_library(
        name = library.name,
        testonly = True,
        srcs = srcs,
        deps = deps,
        alwayslink = 1,
        **kwargs
    )

def failure_lists(name):
    """Declares the directory `<name>/` of this package as its failure lists.

    The directory holds the expected-failure lists of the package's
    conformance_test()s, one file per (test, suite) pair named
    `<test name>_<suite>.txt`, where `<test name>` is the `name` of a
    conformance_test() and `<suite>` one of the conformance suites, e.g.
    `cpp_binary.txt` for `conformance_test(name = "cpp")` and the `binary`
    suite.  A file that doesn't exist means "no expected failures".  Anything
    else in the directory (a file not named that way, a file for an unknown
    suite, any file in a subdirectory) fails the load; a well-formed list that
    no conformance_test() uses (e.g. `foo_binary.txt` without a `foo` test, or
    a list for a suite the test skips) can't be detected, though, and is simply
    never read.  The directory must not be a package of its own (the lists
    are found with `glob`, which doesn't look into subpackages).

    Nothing in the directory should be edited by hand: `blaze run
    :<test name>_<suite>_test -- --fix` creates or rewrites the file for that
    test to match the observed results.  Only the directory itself has to
    exist beforehand; create it (`mkdir`) together with the first list.

    Args:
      name: The name of the directory, and of the filegroup created for it.
        Pass `":" + name` as the `failure_lists` of the package's
        conformance_test()s.
    """
    if native.subpackages(include = [name, name + "/**"], allow_empty = True):
        fail(("failure_lists(%r): %s/ must not contain a BUILD file (it is a directory " +
              "of failure lists, not a package).") % (name, name))
    files = native.glob(["%s/**" % name], allow_empty = True)
    for file in files:
        # The test name may itself contain underscores (e.g. java_lite), so
        # only the suffix can be validated.
        stem = file[len(name) + 1:]
        if "/" in stem or not stem.endswith(".txt"):
            stem = ""
        else:
            stem = stem[:-len(".txt")]
        if not [s for s in _SUITES if stem.endswith("_" + s) and len(stem) > len(s) + 1]:
            fail(("failure_lists(%r): %s doesn't correspond to a conformance suite: failure " +
                  "lists must be named <test name>_<suite>.txt, where <suite> is one of %s.") %
                 (name, file, ", ".join(_SUITES)))
    native.filegroup(
        name = name,
        srcs = files,
    )

def conformance_test(
        name,
        testee,
        failure_lists,
        suites = None,
        maximum_edition = None,
        enforce_recommended = True,
        performance = False,
        in_process = False,
        **kwargs):
    """Runs the conformance suites against a testee.

    Creates one `cc_test` per suite, named `<name>_<suite>_test` (e.g.
    `cpp_binary_test`), and a `test_suite` named `name` running all of them.
    Each test links the suite's library with `:test_environment_main` and runs
    it against `testee` with the failure list
    `<failure_lists directory>/<name>_<suite>.txt` if that file exists;
    otherwise the testee is expected to pass every test in the suite, and
    `blaze run ... -- --fix` creates the file (see failure_lists()).

    By default `testee` is a binary: the test links `:forked_testee`, which
    spawns it as a subprocess speaking the conformance protocol on its
    stdin/stdout, and it is passed as `--testee_binary`.  With `in_process`,
    `testee` is a cc_library that defines MakeTesteeRunner() (see
    testee_runner.h) and is linked into the test instead, so the suites run
    the testee in their own process: debuggable, and no fork or pipe.  Both
    modes take the same failure lists and flags.

    Args:
      name: The test's name; also the prefix of its failure lists.
      testee: The testee: a binary, or with `in_process` a cc_library.
      failure_lists: The failure_lists() of this package.
      suites: The suites to run; default all of them.  Use this to skip a
        suite the testee doesn't support at all (e.g. `text` for a runtime
        without text format).
      maximum_edition: The newest edition to test (e.g. `"2023"`); default
        proto2 and proto3 only.  Passed as `--maximum_edition`.
      enforce_recommended: Whether failures in recommended tests fail the run
        (as in the legacy conformance_test_runner wrapper).  Passed as
        `--enforce_recommended`.
      performance: Whether to run the performance tests instead of the regular
        ones.  A performance test needs its own conformance_test() (and so its
        own failure lists), typically named `<name>_performance`.  Passed as
        `--performance`.
      in_process: Whether `testee` is a cc_library to link into each test
        instead of a binary to spawn.
      **kwargs: Passed through to every cc_test (e.g. `size`, `timeout`,
        `tags`); `tags` also apply to the test_suite.  `args`, `deps` and
        `data` are set by the macro and can't be given.
    """
    if suites == None:
        suites = _SUITES
    for suite in suites:
        _check_suite(suite)

    # The failure lists must be in this package, since only the calling
    # package's files can be globbed to find out which lists exist.
    lists_label = native.package_relative_label(failure_lists)
    if native.package_relative_label(":" + lists_label.name) != lists_label:
        fail(("conformance_test(%r): failure_lists (%s) must be a failure_lists() " +
              "in the same package, //%s.") % (name, failure_lists, native.package_name()))

    for attr in ("args", "deps", "data"):
        if attr in kwargs:
            fail("conformance_test(%r): %s is set by the macro and can't be overridden." %
                 (name, attr))
    tags = kwargs.pop("tags", [])
    tests = []
    for suite in suites:
        test_name = "%s_%s_test" % (name, suite)
        failure_list = "%s/%s_%s.txt" % (lists_label.name, name, suite)

        # The runfiles path of the failure list, which is also its path relative
        # to the workspace root so that --fix can find the source file under
        # $BUILD_WORKSPACE_DIRECTORY.
        failure_list_path = failure_list
        if lists_label.package:
            failure_list_path = "%s/%s" % (lists_label.package, failure_list)

        if in_process:
            # The testee is linked in and would reject --testee_binary (see
            # testee_runner.h).
            args = []
            data = [failure_lists]
            deps = [_suite_library(suite), _TEST_ENVIRONMENT_MAIN, testee]
        else:
            args = ["--testee_binary=$(rootpath %s)" % testee]
            data = [testee, failure_lists]
            deps = [_suite_library(suite), _TEST_ENVIRONMENT_MAIN, _FORKED_TESTEE]

        # (native.glob wouldn't see the file if the failure lists directory ever
        # became a package of its own; failure_lists() forbids that.)
        if native.glob([failure_list], allow_empty = True):
            args.append("--failure_list=%s" % failure_list_path)
        else:
            # No expected failures; tell --fix where to create the list.
            args.append("--fix_output_file=%s" % failure_list_path)
        if maximum_edition:
            args.append("--maximum_edition=%s" % maximum_edition)
        args.append("--enforce_recommended=%s" % ("true" if enforce_recommended else "false"))
        if performance:
            args.append("--performance")

        cc_test(
            name = test_name,
            args = args,
            data = data,
            tags = ["conformance"] + tags,
            deps = deps,
            **kwargs
        )
        tests.append(":" + test_name)

    native.test_suite(
        name = name,
        tests = tests,
        tags = tags,
    )
