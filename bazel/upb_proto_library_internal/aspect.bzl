"""Implementation of the aspect that powers the upb_*_proto_library() rules."""

load("//bazel:upb_proto_library_internal/cc_library_func.bzl", "cc_library_func")
load("//bazel:upb_proto_library_internal/copts.bzl", "UpbProtoLibraryCoptsInfo")
load("@rules_proto//proto:defs.bzl", "proto_common")

# begin:github_only
_is_google3 = False
# end:github_only

# begin:google_only
# _is_google3 = True
# end:google_only

GeneratedSrcsInfo = provider(
    "Provides generated headers and sources",
    fields = {
        "srcs": "list of srcs",
        "hdrs": "list of hdrs",
        "thunks": "Experimental, do not use. List of srcs defining C API. Incompatible with hdrs.",
    },
)

def output_dir(ctx, proto_info):
    """Returns the output directory where generated proto files will be placed.

    Args:
      ctx: Rule context.
      proto_info: ProtoInfo provider.

    Returns:
      A string specifying the output directory
    """
    proto_root = proto_info.proto_source_root
    if proto_root.startswith(ctx.bin_dir.path):
        path = proto_root
    else:
        path = ctx.bin_dir.path + "/" + proto_root

    if proto_root == ".":
        path = ctx.bin_dir.path
    return path

def _concat_lists(lists):
    ret = []
    for lst in lists:
        ret = ret + lst
    return ret

def _merge_generated_srcs(srcs):
    return GeneratedSrcsInfo(
        srcs = _concat_lists([s.srcs for s in srcs]),
        hdrs = _concat_lists([s.hdrs for s in srcs]),
        thunks = _concat_lists([s.thunks for s in srcs]),
    )

def _generate_upb_protos(ctx, generator, proto_info):
    if len(proto_info.direct_sources) == 0:
        return GeneratedSrcsInfo(srcs = [], hdrs = [], thunks = [], includes = [])

    ext = "." + generator
    srcs = []
    thunks = []
    hdrs = proto_common.declare_generated_files(
        ctx.actions,
        extension = ext + ".h",
        proto_info = proto_info,
    )
    if not (generator == "upb" and _is_google3):
        # TODO: The OSS build should also exclude this file for the upb generator,
        # as it is empty and unnecessary.  We only added it to make the OSS build happy on
        # Windows and macOS.
        srcs += proto_common.declare_generated_files(
            ctx.actions,
            extension = ext + ".c",
            proto_info = proto_info,
        )
    if generator == "upb":
        thunks = proto_common.declare_generated_files(
            ctx.actions,
            extension = ext + ".thunks.c",
            proto_info = proto_info,
        )
        ctx.actions.run_shell(
            inputs = hdrs,
            outputs = thunks,
            command = " && ".join([
                "sed 's/UPB_INLINE //' {} > {}".format(hdr.path, thunk.path)
                for (hdr, thunk) in zip(hdrs, thunks)
            ]),
            progress_message = "Generating thunks for upb protos API for: " + ctx.label.name,
            mnemonic = "GenUpbProtosThunks",
        )

    proto_common.compile(
        actions = ctx.actions,
        proto_info = proto_info,
        proto_lang_toolchain_info = _get_lang_toolchain(ctx, generator),
        generated_files = srcs + hdrs,
        experimental_exec_group = "proto_compiler",
    )

    return GeneratedSrcsInfo(
        srcs = srcs,
        hdrs = hdrs,
        thunks = thunks,
    )

def _generate_name(ctx, generator, thunks = False):
    if thunks:
        return ctx.rule.attr.name + "." + generator + ".thunks"
    return ctx.rule.attr.name + "." + generator

def _get_dep_cc_infos(target, ctx, generator, cc_provider, dep_cc_provider):
    rule_deps = ctx.rule.attr.deps
    dep_ccinfos = [dep[cc_provider].cc_info for dep in rule_deps]
    if dep_cc_provider:
        # This gives access to our direct sibling.  eg. foo.upb.h can #include "foo.upb_minitable.h"
        dep_ccinfos.append(target[dep_cc_provider].cc_info)

        # This gives access to imports.  eg. foo.upb.h can #include "import1.upb_minitable.h"
        # But not transitive imports, eg. foo.upb.h cannot #include "transitive_import1.upb_minitable.h"
        dep_ccinfos += [dep[dep_cc_provider].cc_info for dep in rule_deps]

    return dep_ccinfos

def _get_lang_toolchain(ctx, generator):
    lang_toolchain_name = "_" + generator + "_toolchain"
    return getattr(ctx.attr, lang_toolchain_name)[proto_common.ProtoLangToolchainInfo]

def _compile_upb_protos(ctx, files, generator, dep_ccinfos, cc_provider, proto_info):
    cc_info = cc_library_func(
        ctx = ctx,
        name = _generate_name(ctx, generator),
        hdrs = files.hdrs,
        srcs = files.srcs,
        includes = [output_dir(ctx, proto_info)],
        copts = ctx.attr._copts[UpbProtoLibraryCoptsInfo].copts,
        dep_ccinfos = dep_ccinfos,
    )

    if files.thunks:
        cc_info_with_thunks = cc_library_func(
            ctx = ctx,
            name = _generate_name(ctx, generator, files.thunks),
            hdrs = [],
            srcs = files.thunks,
            includes = [output_dir(ctx, proto_info)],
            copts = ctx.attr._copts[UpbProtoLibraryCoptsInfo].copts,
            dep_ccinfos = dep_ccinfos + [cc_info],
        )
        return cc_provider(
            cc_info = cc_info,
            cc_info_with_thunks = cc_info_with_thunks,
        )
    else:
        return cc_provider(
            cc_info = cc_info,
        )

_GENERATORS = ["upb", "upbdefs", "upb_minitable"]

def _get_hint_providers(ctx, generator):
    if generator not in _GENERATORS:
        fail("Please add new generator '{}' to _GENERATORS list".format(generator))

    possible_owners = []
    for generator in _GENERATORS:
        possible_owners.append(ctx.label.relative(_generate_name(ctx, generator)))
        possible_owners.append(ctx.label.relative(_generate_name(ctx, generator, thunks = True)))

    if hasattr(cc_common, "CcSharedLibraryHintInfo"):
        return [cc_common.CcSharedLibraryHintInfo(owners = possible_owners)]
    elif hasattr(cc_common, "CcSharedLibraryHintInfo_6_X_constructor_do_not_use"):
        # This branch can be deleted once 6.X is not supported by upb rules
        return [cc_common.CcSharedLibraryHintInfo_6_X_constructor_do_not_use(owners = possible_owners)]

    return []

def upb_proto_aspect_impl(
        target,
        ctx,
        generator,
        cc_provider,
        dep_cc_provider,
        file_provider,
        provide_cc_shared_library_hints = True):
    """A shared aspect implementation for upb_*proto_library() rules.

    Args:
      target: The `target` parameter from the aspect function.
      ctx: The `ctx` parameter from the aspect function.
      generator: A string describing which aspect we are generating. This triggers several special
        behaviors, and ideally this will be refactored to be less magical.
      cc_provider: The provider that this aspect will attach to the target. Should contain a
        `cc_info` field. The aspect will ensure that each compilation action can compile and link
        against this provider's cc_info for all proto_library() deps.
      dep_cc_provider: For aspects that depend on other aspects, this is the provider of the aspect
        that we depend on. The aspect wil be able to include the header files from this provider.
      file_provider: A provider that this aspect will attach to the target to expose the source
        files generated by this aspect. These files are primarily useful for returning in
        DefaultInfo(), so users who build the upb_*proto_library() rule directly can view the
        generated sources.
      provide_cc_shared_library_hints: Whether shared library hints should be provided.

    Returns:
      The `cc_provider` and `file_provider` providers as described above.
    """
    dep_ccinfos = _get_dep_cc_infos(target, ctx, generator, cc_provider, dep_cc_provider)
    if not getattr(ctx.rule.attr, "srcs", []):
        # This target doesn't declare any sources, reexport all its deps instead.
        # This is known as an "alias library":
        #    https://bazel.build/reference/be/protocol-buffer#proto_library.srcs
        files = _merge_generated_srcs([dep[file_provider].srcs for dep in ctx.rule.attr.deps])
        wrapped_cc_info = cc_provider(
            cc_info = cc_common.merge_cc_infos(direct_cc_infos = dep_ccinfos),
        )
    else:
        proto_info = target[ProtoInfo]
        files = _generate_upb_protos(
            ctx,
            generator,
            proto_info,
        )
        wrapped_cc_info = _compile_upb_protos(
            ctx,
            files,
            generator,
            dep_ccinfos + [_get_lang_toolchain(ctx, generator).runtime[CcInfo]],
            cc_provider,
            proto_info,
        )

    hints = _get_hint_providers(ctx, generator) if provide_cc_shared_library_hints else []

    return hints + [
        file_provider(srcs = files),
        wrapped_cc_info,
    ]
