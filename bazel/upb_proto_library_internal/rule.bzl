"""Internal rule implementation for upb_*_proto_library() rules."""

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
    return [
        DefaultInfo(files = depset(files + srcs.hdrs + srcs.srcs)),
        srcs,
        cc_info,
    ]
