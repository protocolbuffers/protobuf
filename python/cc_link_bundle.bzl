"""Bazel 8 workaround: bundle all of `_message`'s deps into a single cc_shared_library node.

Only used in OSS, where `_message` is built with rules_python's py_extension, which links
with cc_shared_library.

Bazel 8's builtin cc_shared_library only links a linker input if its owner is a node that
its graph_structure_aspect visited. That aspect does not visit proto_library targets when
protobuf is the root module, because it only recognizes ProtoInfo from a hardcoded list of
canonical repo names (@com_google_protobuf, @com_google_protobuf+, @@protobuf+). The upb
aspects' generated code is owned by synthetic labels like `foo_proto.upb_minitable`, which
upb's CcSharedLibraryHintInfo maps to the proto_library node. Since that node is never
visited, the generated code is silently dropped from the link.

This rule re-owns every linker input of its deps under its own label. The bundle provides
CcInfo, so Bazel 8's cc_shared_library visits it, and it then treats the whole dependency
closure as one statically-linked node, which is all `_message` needs: it is a single leaf .so
with no dynamic_deps.

The linker inputs are flattened exactly once, here, so each library appears exactly once
and the topological order of the merged depset is preserved.

This must not be used where the re-owned linker inputs could be merged with the original
ones (e.g. a binary that also depends on other upb protos directly): the copies would not be
deduplicated and the link order could become invalid.

Not needed on Bazel 9 (rules_cc's cc_shared_library recognizes protobuf's ProtoInfo) or
when protobuf is not the root module. Delete once protobuf drops Bazel 8 support.
"""

# All loads are inside these copybara blocks (rather than shared), so that load sorting by
# buildifier cannot move a shared load into the stripped block.
load("@proto_bazel_features//:features.bzl", "bazel_features")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_python//python/cc:py_extension.bzl", "py_extension")

# Only Bazel 8's builtin cc_shared_library needs the bundle. Delete this file once protobuf
# drops Bazel 8 support.
_NEEDS_BUNDLE = bazel_features.cc.builtin_cc_shared_library

def _cc_link_bundle_impl(ctx):
    # direct_cc_infos: re-export the deps' headers as this target's own, so that targets
    # depending on the bundle still pass layering_check for headers from the bundled deps.
    cc_info = cc_common.merge_cc_infos(direct_cc_infos = [dep[CcInfo] for dep in ctx.attr.deps])
    linker_inputs = [
        cc_common.create_linker_input(
            owner = ctx.label,
            libraries = depset(li.libraries),
            user_link_flags = depset(li.user_link_flags),
            additional_inputs = depset(li.additional_inputs),
        )
        for li in cc_info.linking_context.linker_inputs.to_list()
    ]
    return [
        CcInfo(
            compilation_context = cc_info.compilation_context,
            linking_context = cc_common.create_linking_context(
                linker_inputs = depset(linker_inputs, order = "topological"),
            ),
        ),
    ]

_cc_link_bundle = rule(
    implementation = _cc_link_bundle_impl,
    attrs = {
        "deps": attr.label_list(providers = [CcInfo]),
    },
    provides = [CcInfo],
    doc = "Re-owns all deps' linker inputs under this target, for cc_shared_library on Bazel 8.",
)

def py_extension_with_bundled_deps(name, deps = [], **kwargs):
    """A py_extension whose deps are linked as a single cc_link_bundle node (Bazel 8 only).

    On Blaze and Bazel 9+ this is exactly py_extension().

    Args:
      name: Name of the py_extension target.
      deps: Dependencies of the py_extension. On Bazel 8 (OSS), these are linked through a
        single _cc_link_bundle target named `<name>_deps`.
      **kwargs: Other attributes, passed through to py_extension.
    """
    if _NEEDS_BUNDLE:
        _cc_link_bundle(
            name = name + "_deps",
            deps = deps,
            target_compatible_with = kwargs.get("target_compatible_with"),
            visibility = ["//visibility:private"],
        )

        # Replace (don't extend) deps: each library must be linked under exactly one owner.
        deps = [":" + name + "_deps"]

    py_extension(name = name, deps = deps, **kwargs)
