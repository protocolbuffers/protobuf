"""This file implements rust_proto_library (frontend macro)."""

# go/for-macros-only

load(
    ":rules.bzl",
    _ProtoCrateNamesInfo = "ProtoCrateNamesInfo",
    _rust_cc_proto_library = "rust_cc_proto_library",
    _rust_upb_proto_library = "rust_upb_proto_library",
)

ProtoCrateNamesInfo = _ProtoCrateNamesInfo
rust_upb_proto_library = _rust_upb_proto_library
rust_cc_proto_library = _rust_cc_proto_library

def _rust_proto_library_impl(name, deps, visibility = None, tags = None, **kwargs):
    """Declares all the boilerplate needed to use Rust protobufs conveniently.

    Hopefully no user will ever need to read this code.

    Args:
        name: name of the Rust protobuf target.
        deps: proto_library target for which to generate Rust gencode.
        visibility: visibility of the generated targets.
        tags: tags of the generated targets.
        **kwargs: other args passed to the rust_<kernel>_proto_library targets.
    """
    if not name.endswith("_rust_proto"):
        fail(
            "Name rust_proto_library target should end with `_rust_proto`, but was '{}'"
                .format(name),
        )

    native.alias(
        name = name,
        actual = select({
            Label("//rust:use_upb_kernel"): name + "_upb",
            "//conditions:default": name + "_cpp",
        }),
        visibility = visibility,
        tags = tags,
    )

    rust_upb_proto_library(
        name = name + "_upb",
        deps = deps,
        visibility = ["//rust/test:__subpackages__"],
        tags = tags,
        **kwargs
    )

    rust_cc_proto_library(
        name = name + "_cpp",
        deps = deps,
        visibility = ["//rust/test:__subpackages__"],
        tags = tags,
        **kwargs
    )

rust_proto_library = macro(
    implementation = _rust_proto_library_impl,
    inherit_attrs = rust_upb_proto_library,
    doc = """Declares all the boilerplate needed to use Rust protobufs conveniently.

    Hopefully no user will ever need to read this code.
    """,
)
