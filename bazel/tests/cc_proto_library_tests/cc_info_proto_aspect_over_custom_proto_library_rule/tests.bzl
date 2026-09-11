"""Regression test for bugs b/160342669 and b/182353581."""

load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("//bazel/common:proto_info.bzl", "ProtoInfo")

def _provides_proto_info_impl(ctx):
    proto_info = ctx.attr.proto_dep[ProtoInfo]
    return [proto_info]

provides_proto_info = rule(
    implementation = _provides_proto_info_impl,
    attrs = {
        "proto_dep": attr.label(),
    },
    provides = [ProtoInfo],
)

def _test_custom_proto_library_rule(env, target):
    # The test passes if the target analysis succeeded and provides CcInfo.
    env.expect.that_target(target).has_provider(CcInfo)

TESTS = {
    ":consumer_cc_proto": [_test_custom_proto_library_rule],
}
