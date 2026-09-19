# Protocol Buffers - Google's data interchange format
# Copyright 2025 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""Analysis tests for the conformance_test() macro in conformance.bzl.

These check the targets the macro generates and the command lines it passes
to them.  The `fail()` paths (unknown suite, misnamed failure list, foreign
failure_lists) happen at loading time and so can't be covered here.

The tests live in their own leaf package (bzl_tests) because they load
rules_testing, which is only a dev dependency of the open-source build; the
exported conformance package must not depend on it.
"""

load("@rules_testing//lib:analysis_test.bzl", "analysis_test")
load("@rules_testing//lib:truth.bzl", "subjects")
load("@rules_testing//lib:util.bzl", "util")
load("//conformance:conformance.bzl", "conformance_test")

# The failure_lists() the subjects use, and the stub testees (a binary and an
# in-process library); see BUILD.
_FAILURE_LISTS = "bzl_test_failure_lists"
_TESTEE = ":stub_testee"
_IN_PROCESS_TESTEE = ":stub_in_process_testee"

def conformance_bzl_test_suite(name):
    """Creates the analysis tests for conformance_test() and a suite of them.

    Every generated target is prefixed with `name`.  The `with_failure_list`
    test needs the fixtures `<name>_with_failure_list_subject_binary.txt` and
    `<name>_with_failure_list_subject_performance.txt` in the package's
    failure lists directory.

    Args:
      name: The name of the test_suite, and the prefix of its tests.
    """
    tests = []
    for suffix, setup in [
        ("with_failure_list", _test_with_failure_list),
        ("without_failure_list", _test_without_failure_list),
        ("options", _test_options),
        ("in_process", _test_in_process),
    ]:
        test_name = "%s_%s" % (name, suffix)
        setup(test_name)
        tests.append(":" + test_name)
    native.test_suite(
        name = name,
        tests = tests,
    )

# The conformance_test() under test can't be named after the analysis test,
# since its test_suite would clash with it.
def _subject(name):
    return name + "_subject"

def _subject_of(target, suite = "binary"):
    """The name of the conformance_test() that generated `target`, its `suite` test."""
    return target.label.name.removesuffix("_%s_test" % suite)

def _failure_list_path(target, suite = "binary"):
    """The workspace-relative path of `target`'s failure list; `target` is its `suite` test."""
    return "%s/%s/%s_%s.txt" % (
        target.label.package,
        _FAILURE_LISTS,
        _subject_of(target, suite),
        suite,
    )

def _label_names(value, *, meta):
    """Subject factory for a label list attribute, comparing target names.

    Labels from other repositories are ignored: those are implicit dependencies
    added by the rule itself (e.g. @bazel_tools//tools/cpp:link_extra_lib on a
    cc_test), not by the macro under test.
    """
    return subjects.collection(
        [target.label.name for target in value if not target.label.repo_name],
        meta = meta,
    )

def _test_with_failure_list(name):
    for suite in ("binary", "performance"):
        fixture = "%s/%s_%s.txt" % (_FAILURE_LISTS, _subject(name), suite)
        if not native.glob([fixture], allow_empty = True):
            fail("%s: the fixture failure list %s doesn't exist." % (name, fixture))
    util.helper_target(
        conformance_test,
        name = _subject(name),
        testee = _TESTEE,
        failure_lists = ":" + _FAILURE_LISTS,
    )
    analysis_test(
        name = name,
        targets = {
            "suite": _subject(name),
            "test": _subject(name) + "_binary_test",
            "text_test": _subject(name) + "_text_test",
            "performance_test": _subject(name) + "_performance_test",
        },
        impl = _test_with_failure_list_impl,
    )

def _test_with_failure_list_impl(env, targets):
    test = env.expect.that_target(targets.test)
    failure_list = _failure_list_path(targets.test)
    test.attr("args", factory = subjects.collection).contains_exactly([
        "--testee_binary=$(rootpath %s)" % _TESTEE,
        "--failure_list=" + failure_list,
        "--enforce_recommended=true",
    ]).in_order()
    test.attr("deps", factory = _label_names).contains_exactly([
        "binary_conformance_tests",
        "test_environment_main",
        "forked_testee",
    ])
    test.tags().contains("conformance")
    test.runfiles().contains_at_least([
        "{workspace}/" + failure_list,
        "{workspace}/{package}/stub_testee",
    ])

    # The text suite's test links that suite instead; it has no fixture list,
    # so --fix is told where to create one (see _test_without_failure_list).
    text_test = env.expect.that_target(targets.text_test)
    text_test.attr("args", factory = subjects.collection).contains_exactly([
        "--testee_binary=$(rootpath %s)" % _TESTEE,
        "--fix_output_file=" + _failure_list_path(targets.text_test, suite = "text"),
        "--enforce_recommended=true",
    ]).in_order()
    text_test.attr("deps", factory = _label_names).contains_exactly([
        "text_conformance_tests",
        "test_environment_main",
        "forked_testee",
    ])

    # The performance suite is one of the default suites: it gets a test of
    # its own, built like the others from its own library and failure list.
    performance_test = env.expect.that_target(targets.performance_test)
    performance_failure_list = _failure_list_path(targets.performance_test, suite = "performance")
    performance_test.attr("args", factory = subjects.collection).contains_exactly([
        "--testee_binary=$(rootpath %s)" % _TESTEE,
        "--failure_list=" + performance_failure_list,
        "--enforce_recommended=true",
    ]).in_order()
    performance_test.attr("deps", factory = _label_names).contains_exactly([
        "performance_conformance_tests",
        "test_environment_main",
        "forked_testee",
    ])
    performance_test.runfiles().contains_at_least([
        "{workspace}/" + performance_failure_list,
    ])

    # No suite is kept off the sanitizers any more: the macro adds no tag but
    # "conformance" to the caller's.
    performance_test.tags().contains("conformance")
    performance_test.tags().not_contains("noasan")
    test.tags().not_contains("noasan")

    # One test per known conformance suite (see _SUITES in conformance.bzl).
    suite = env.expect.that_target(targets.suite)
    suite.attr("tests", factory = _label_names).contains_exactly([
        _subject_of(targets.test) + "_binary_test",
        _subject_of(targets.test) + "_text_test",
        _subject_of(targets.test) + "_json_test",
        _subject_of(targets.test) + "_performance_test",
    ])

    # helper_target tags the tests manual (among others) so that they never run
    # outside these analysis tests; the tags must reach the test_suite too.
    suite.tags().contains("manual")

def _test_without_failure_list(name):
    util.helper_target(
        conformance_test,
        name = _subject(name),
        testee = _TESTEE,
        failure_lists = ":" + _FAILURE_LISTS,
    )
    analysis_test(
        name = name,
        target = _subject(name) + "_binary_test",
        impl = _test_without_failure_list_impl,
    )

def _test_without_failure_list_impl(env, target):
    # With no failure list, --fix is told where to create one instead.
    env.expect.that_target(target).attr("args", factory = subjects.collection).contains_exactly([
        "--testee_binary=$(rootpath %s)" % _TESTEE,
        "--fix_output_file=" + _failure_list_path(target),
        "--enforce_recommended=true",
    ]).in_order()

def _test_options(name):
    util.helper_target(
        conformance_test,
        name = _subject(name),
        testee = _TESTEE,
        failure_lists = ":" + _FAILURE_LISTS,
        suites = ["binary"],
        maximum_edition = "2023",
        enforce_recommended = False,
        tags = ["noasan"],
    )
    analysis_test(
        name = name,
        target = _subject(name) + "_binary_test",
        impl = _test_options_impl,
    )

def _test_options_impl(env, target):
    test = env.expect.that_target(target)
    test.attr("args", factory = subjects.collection).contains_exactly([
        "--testee_binary=$(rootpath %s)" % _TESTEE,
        "--fix_output_file=" + _failure_list_path(target),
        "--maximum_edition=2023",
        "--enforce_recommended=false",
    ]).in_order()
    test.tags().contains_at_least(["conformance", "noasan"])

def _test_in_process(name):
    util.helper_target(
        conformance_test,
        name = _subject(name),
        testee = _IN_PROCESS_TESTEE,
        failure_lists = ":" + _FAILURE_LISTS,
        suites = ["binary"],
        in_process = True,
    )
    analysis_test(
        name = name,
        target = _subject(name) + "_binary_test",
        impl = _test_in_process_impl,
    )

def _test_in_process_impl(env, target):
    # The testee is linked in place of forked_testee and gets no
    # --testee_binary; the other flags are unchanged.
    test = env.expect.that_target(target)
    test.attr("args", factory = subjects.collection).contains_exactly([
        "--fix_output_file=" + _failure_list_path(target),
        "--enforce_recommended=true",
    ]).in_order()
    test.attr("deps", factory = _label_names).contains_exactly([
        "binary_conformance_tests",
        "test_environment_main",
        "stub_in_process_testee",
    ])
    test.attr("data", factory = _label_names).contains_exactly([_FAILURE_LISTS])
    test.runfiles().not_contains("{workspace}/{package}/stub_testee")
    test.tags().contains("conformance")
