"""Regression test for bug b/158509711: NullPointerException caused by attribute.getDefaultValue(null) in StarlarkRuleContext.<init>."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _rule_impl(ctx):  # buildifier: disable=unused-variable
    return []

def _aspect_impl(target, ctx):  # buildifier: disable=unused-variable
    _ = [] + getattr(ctx.rule.attr, "_default_copts", [])  # buildifier: disable=unused-variable
    return []

_aspect = aspect(
    implementation = _aspect_impl,
    attr_aspects = ["deps"],
    required_aspect_providers = [CcInfo],
)

cc_equals_proto_library = rule(
    implementation = _rule_impl,
    attrs = {"deps": attr.label_list(aspects = [_aspect])},
)

def _test_starlark_aspect_over_cc_proto_library_aspect(env, target):
    # If the target is successfully configured and analyzed, the aspect ran without NPE.
    env.expect.that_target(target)

TESTS = {
    ":foo_cc_equals_proto": [_test_starlark_aspect_over_cc_proto_library_aspect],
}
