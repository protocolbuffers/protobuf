"""TODO: mkruskal - Write module docstring."""

# TODO: figure out a way to keep this in sync with the suites defined in BUILD.
_CONFORMANCE_SUITES = ["binary", "text"]

def conformance_suite(name, srcs, deps = []):
    if name not in _CONFORMANCE_SUITES:
        fail("Invalid suite name: %s" % name)

    native.cc_test(
        name = name + "_test",
        srcs = srcs,
        deps = deps + [
            ":global_test_environment_main",  # This prevents aggregation of tests.
        ],
        tags = ["manual"],
    )

def failure_lists(name):
    # This machinery is necessary to allow for non-existent failure lists.  If we add a new suite,
    # we don't want to have to add a bunch of empty failure lists.  This filegroup makes all of the
    # relevant failure lists available to the conformance test, and we simply tell it which one to
    # pick out (or create if --fix is used).
    native.filegroup(
        name = name,
        srcs = native.glob(["%s/*.txt" % name]),
    )
    # TODO: Is this even any better?  It means that *extra* failure lists will just silently be
    # ignored forever.  It might be better to be more explicit.

def conformance_test(name, testee, failure_lists, **kwargs):
    # TODO: Provide an option to link in an in-process testee instead of a binary.  This will allow
    # us to run the tests in a single process for C and C++ tests, which will be faster and more
    # debuggable.

    # TODO: Provide an option to merge all suites into a single binary.  This should be doable as
    # long as we split out libraries that don't link in global_test_environment_main.  This could be
    # useful for bulk-updates of failure lists.

    tests = []
    failure_list_label = native.package_relative_label(failure_lists)
    for suite in _CONFORMANCE_SUITES:
        failure_list = "%s/%s/%s_%s.txt" % (failure_list_label.package, failure_list_label.name, name, suite)
        tests.append("%s_%s_test" % (name, suite))
        native.sh_test(
            name = "%s_%s_test" % (name, suite),
            srcs = [":%s_test" % suite],
            tags = ["manual"],
            args = [
                "--testee_binary=$(location %s)" % testee,
                "--expected_failures_list=%s" % (failure_list),
            ],
            data = [
                testee,
                failure_lists,
            ],
        )
    native.test_suite(
        name = name,
        tests = tests,
    )
