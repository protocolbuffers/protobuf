"""Internal rule implementation for upb_*_proto_library() rules."""

load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")

def _filter_none(elems):
    out = []
    for elem in elems:
        if elem:
            out.append(elem)
    return out

def upb_proto_rule_impl(ctx, cc_info_provider, srcs_provider):
    """An implementation for upb_*proto_library() rules.

    Args:
      ctx: The rule `ctx` argument
      cc_info_provider: The provider containing a wrapped CcInfo that will be exposed to users who
        depend on this rule.
      srcs_provider: The provider containing the generated source files. This will be used to make
        the DefaultInfo return the source files.

    Returns:
      Providers for this rule.
    """
    if len(ctx.attr.deps) != 1:
        fail("only one deps dependency allowed.")
    dep = ctx.attr.deps[0]
    srcs = dep[srcs_provider].srcs
    cc_info = dep[cc_info_provider].cc_info

    lib = cc_info.linking_context.linker_inputs.to_list()[0].libraries[0]
    files = _filter_none([
        lib.static_library,
        lib.pic_static_library,
        lib.dynamic_library,
    ])

    linker_inputs = []
    dep_prefix = dep.label.name + "."
    for input in cc_info.linking_context.linker_inputs.to_list():
        if input.owner == dep.label or (input.owner.package == dep.label.package and input.owner.name.startswith(dep_prefix)):
            linker_inputs.append(cc_common.create_linker_input(
                owner = ctx.label,
                libraries = depset(input.libraries),
                user_link_flags = depset(input.user_link_flags),
                additional_inputs = depset(input.additional_inputs),
            ))
        else:
            linker_inputs.append(input)

    linking_context = cc_common.create_linking_context(
        linker_inputs = depset(linker_inputs, order = "topological"),
    )
    cc_info = CcInfo(
        compilation_context = cc_info.compilation_context,
        linking_context = linking_context,
    )

    return [
        DefaultInfo(files = depset(files + srcs.hdrs + srcs.srcs)),
        srcs,
        cc_info,
    ]
