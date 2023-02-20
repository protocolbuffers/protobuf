"""Support for rust_proto_library_aspect unit-tests."""

load("//rust:defs.bzl", "RustProtoInfo", "rust_proto_library_aspect")

ActionsInfo = provider(
    doc = ("A provider that exposes what actions were registered by rust_proto_library_aspect " +
           "on proto_libraries."),
    fields = {"actions": "List[Action]: actions registered on proto_libraries."},
)

def _attach_aspect_impl(ctx):
    return [ctx.attr.dep[RustProtoInfo], ActionsInfo(actions = ctx.attr.dep.actions)]

attach_aspect = rule(
    implementation = _attach_aspect_impl,
    attrs = {
        "dep": attr.label(aspects = [rust_proto_library_aspect]),
    },
)
