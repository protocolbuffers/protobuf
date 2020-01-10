"""Contains a unittest to verify that `cc_proto_library` does not generate code for blacklisted `.proto` sources (i.e. WKPs)."""

load("@bazel_skylib//lib:unittest.bzl", "asserts", "unittest")

def _cc_proto_blacklist_test_impl(ctx):
    """Verifies that there are no C++ compile actions for Well-Known-Protos.

    Args:
      ctx: The rule context.

    Returns: A (not further specified) sequence of providers.
    """

    env = unittest.begin(ctx)

    for dep in ctx.attr.deps:
        files = len(dep.files.to_list())
        asserts.equals(
            env,
            0,
            files,
            "Expected that target '{}' does not provide files, got {}".format(
                dep.label,
                files,
            ),
        )

    return unittest.end(env)

cc_proto_blacklist_test = unittest.make(
    impl = _cc_proto_blacklist_test_impl,
    attrs = {
        "deps": attr.label_list(
            mandatory = True,
            providers = [CcInfo],
        ),
    },
)
