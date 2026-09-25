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

# The failure_lists() the subjects use, and the stub testee; see BUILD.
_FAILURE_LISTS = "bzl_test_failure_lists"
_TESTEE = ":stub_testee"

def conformance_bzl_test_suite(name):
    """Creates the analysis tests for conformance_test() and a suite of them.

    Every generated target is prefixed with `name`.  The `with_failure_list`
    test needs the fixture `<name>_with_failure_list_subject_binary.txt` in
    the package's failure lists directory.

    Args:
      name: The name of the test_suite, and the prefix of its tests.
    """
    tests = []
    for suffix, setup in [
        ("with_failure_list", _test_with_failure_list),
        ("without_failure_list", _test_without_failure_list),
        ("options", _test_options),
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

def _subject_of(target):
    """The name of the conformance_test() that generated `target`."""
    return target.label.name.removesuffix("_binary_test")

def _failure_list_path(target):
    """The workspace-relative path of `target`'s binary failure list."""
    return "%s/%s/%s_binary.txt" % (target.label.package, _FAILURE_LISTS, _subject_of(target))

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
    fixture = "%s/%s_binary.txt" % (_FAILURE_LISTS, _subject(name))
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
            "testee": _TESTEE,
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
    ])
    test.tags().contains("conformance")

    # The testee's path is taken from the target rather than spelled out so
    # that the check also holds where the binary is stub_testee.exe.
    test.runfiles().contains_at_least([
        "{workspace}/" + failure_list,
        "{workspace}/" + targets.testee[DefaultInfo].files_to_run.executable.short_path,
    ])

    suite = env.expect.that_target(targets.suite)
    suite.attr("tests", factory = _label_names).contains_exactly([
        _subject_of(targets.test) + "_binary_test",
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
        performance = True,
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
        "--performance",
    ]).in_order()
    test.tags().contains_at_least(["conformance", "noasan"])
