"""Starlark definitions for Protobuf conformance tests.

PLEASE DO NOT DEPEND ON THE CONTENTS OF THIS FILE, IT IS UNSTABLE.
"""

load("@rules_shell//shell:sh_test.bzl", "sh_test")

def conformance_test(
        name,
        testee,
        failure_list = None,
        text_format_failure_list = None,
        maximum_edition = None,
        **kwargs):
    """Conformance test runner.

    Args:
      name: the name for the test.
      testee: a conformance test client binary.
      failure_list: a text file with known failures, one per line.
      text_format_failure_list: a text file with known failures (one per line)
          for the text format conformance suite.
      **kwargs: common arguments to pass to sh_test.
    """
    args = ["--testee %s" % _strip_bazel(testee)]
    failure_lists = []
    if failure_list:
        args = args + ["--failure_list %s" % _strip_bazel(failure_list)]
        failure_lists = failure_lists + [failure_list]
    if text_format_failure_list:
        args = args + ["--text_format_failure_list %s" % _strip_bazel(text_format_failure_list)]
        failure_lists = failure_lists + [text_format_failure_list]
    if maximum_edition:
        args = args + ["--maximum_edition %s" % maximum_edition]

    sh_test(
        name = name,
        srcs = ["//conformance:bazel_conformance_test_runner.sh"],
        data = [testee] + failure_lists + [
            "//conformance:conformance_test_runner",
        ],
        args = args,
        deps = [
            "@bazel_tools//tools/bash/runfiles",
        ],
        tags = ["conformance"],
        **kwargs
    )

def _strip_bazel(testee):
    if testee.startswith("//"):
        testee = testee.replace("//", "com_google_protobuf/")
    return testee.replace(":", "/")
