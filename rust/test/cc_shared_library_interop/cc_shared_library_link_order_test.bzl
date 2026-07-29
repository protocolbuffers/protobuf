"""Unit tests for cc_shared_library link order verification."""

load("@bazel_skylib//lib:unittest.bzl", "analysistest", "asserts")

def _cc_shared_library_link_order_test_impl(ctx):
    env = analysistest.begin(ctx)
    target_under_test = analysistest.target_under_test(env)

    # Find the CppLink action
    link_action = None
    for action in target_under_test.actions:
        if action.mnemonic == "CppLink":
            link_action = action
            break

    asserts.true(env, link_action != None, "CppLink action not found")

    argv = link_action.argv

    # Helper to find index of an argument matching a substring
    def find_index(pattern):
        for i, arg in enumerate(argv):
            if pattern in arg:
                return i
        return -1

    bar_idx = find_index("libbar-")
    foo_proto_idx = find_index("libfoo_proto-")
    cpp_api_idx = find_index("cpp_api/")

    asserts.true(env, bar_idx != -1, "libbar not found in linker arguments")
    asserts.true(env, foo_proto_idx != -1, "libfoo_proto not found in linker arguments")
    asserts.true(env, cpp_api_idx != -1, "cpp_api not found in linker arguments")

    # We want: bar comes before foo_proto, which comes before cpp_api
    # (dependencies come after in linker command line)
    asserts.true(
        env,
        bar_idx < foo_proto_idx,
        "Expected libbar to come before libfoo_proto in linker arguments, but got: bar_idx=%d, foo_proto_idx=%d" % (bar_idx, foo_proto_idx),
    )
    asserts.true(
        env,
        foo_proto_idx < cpp_api_idx,
        "Expected libfoo_proto to come before cpp_api in linker arguments, but got: foo_proto_idx=%d, cpp_api_idx=%d" % (foo_proto_idx, cpp_api_idx),
    )

    return analysistest.end(env)

cc_shared_library_link_order_test = analysistest.make(
    impl = _cc_shared_library_link_order_test_impl,
)
