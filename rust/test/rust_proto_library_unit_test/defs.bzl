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

def _attach_cc_aspect_impl(ctx):
    return [ctx.attr.dep[RustProtoInfo], ActionsInfo(actions = ctx.attr.dep.actions)]

attach_cc_aspect = rule(
    implementation = _attach_cc_aspect_impl,
    attrs = {
        "dep": attr.label(aspects = [rust_cc_proto_library_aspect]),
    },
)
