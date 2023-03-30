"""Support for rust_proto_library_aspect unit-tests."""

load(
    "//rust:aspects.bzl",
    "RustProtoInfo",
    "rust_cc_proto_library_aspect",
    "rust_upb_proto_library_aspect",
)

ActionsInfo = provider(
    doc = ("A provider that exposes what actions were registered by rust_proto_library aspects " +
           "on proto_libraries."),
    fields = {"actions": "List[Action]: actions registered on proto_libraries."},
)

def _attach_upb_aspect_impl(ctx):
    return [ctx.attr.dep[RustProtoInfo], ActionsInfo(actions = ctx.attr.dep.actions)]

attach_upb_aspect = rule(
    implementation = _attach_upb_aspect_impl,
    attrs = {
        "dep": attr.label(aspects = [rust_upb_proto_library_aspect]),
    },
)

CcAspectHelperInfo = provider(
    fields = {
        "rust_proto_info": "RustProtoInfo from the proto_library",
        "actions_info": "Actions of the proto_library",
    },
    doc = "A provider passing data from proto_library through cc_proto_library",
)

def _cc_aspect_helper_impl(_target, ctx):
    if ctx.rule.kind == "cc_proto_library":
        return CcAspectHelperInfo(
            rust_proto_info = ctx.rule.attr.deps[0][RustProtoInfo],
            actions_info = ActionsInfo(actions = ctx.rule.attr.deps[0].actions),
        )

    return []

_cc_aspect_helper = aspect(
    implementation = _cc_aspect_helper_impl,
    requires = [rust_cc_proto_library_aspect],
    attr_aspects = ["deps"],
)

def _attach_cc_aspect_impl(ctx):
    helper = ctx.attr.dep[CcAspectHelperInfo]
    return [helper.rust_proto_info, helper.actions_info]

attach_cc_aspect = rule(
    implementation = _attach_cc_aspect_impl,
    attrs = {
        "dep": attr.label(aspects = [_cc_aspect_helper]),
    },
)
