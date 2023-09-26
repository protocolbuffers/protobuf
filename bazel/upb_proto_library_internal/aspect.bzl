"""Implementation of the aspect that powers the upb_*_proto_library() rules."""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("//bazel:upb_proto_library_internal/cc_library_func.bzl", "cc_library_func")
load("//bazel:upb_proto_library_internal/copts.bzl", "UpbProtoLibraryCoptsInfo")

# begin:github_only
_is_google3 = False
# end:github_only

# begin:google_only
# _is_google3 = True
# end:google_only

def _get_real_short_path(file):
    # For some reason, files from other archives have short paths that look like:
    #   ../com_google_protobuf/google/protobuf/descriptor.proto
    short_path = file.short_path
    if short_path.startswith("../"):
        second_slash = short_path.index("/", 3)
        short_path = short_path[second_slash + 1:]

    # Sometimes it has another few prefixes like:
    #   _virtual_imports/any_proto/google/protobuf/any.proto
    #   benchmarks/_virtual_imports/100_msgs_proto/benchmarks/100_msgs.proto
    # We want just google/protobuf/any.proto.
    virtual_imports = "_virtual_imports/"
    if virtual_imports in short_path:
        short_path = short_path.split(virtual_imports)[1].split("/", 1)[1]
    return short_path

def _get_real_root(ctx, file):
    real_short_path = _get_real_short_path(file)
    root = file.path[:-len(real_short_path) - 1]

    if not _is_google3 and ctx.rule.attr.strip_import_prefix:
        root = paths.join(root, ctx.rule.attr.strip_import_prefix[1:])
    return root

def _generate_output_file(ctx, src, extension):
    package = ctx.label.package
    if not _is_google3:
        strip_import_prefix = ctx.rule.attr.strip_import_prefix
        if strip_import_prefix and strip_import_prefix != "/":
            if not package.startswith(strip_import_prefix[1:]):
                fail("%s does not begin with prefix %s" % (package, strip_import_prefix))
            package = package[len(strip_import_prefix):]

    real_short_path = _get_real_short_path(src)
    real_short_path = paths.relativize(real_short_path, package)
    output_filename = paths.replace_extension(real_short_path, extension)
    ret = ctx.actions.declare_file(output_filename)
    return ret

def _generate_include_path(src, out, extension):
    short_path = _get_real_short_path(src)
    short_path = paths.replace_extension(short_path, extension)
    if not out.path.endswith(short_path):
        fail("%s does not end with %s" % (out.path, short_path))

    return out.path[:-len(short_path)]

GeneratedSrcsInfo = provider(
    "Provides generated headers and sources",
    fields = {
        "srcs": "list of srcs",
        "hdrs": "list of hdrs",
        "thunks": "Experimental, do not use. List of srcs defining C API. Incompatible with hdrs.",
        "includes": "list of extra includes",
    },
)

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
        includes = _concat_lists([s.includes for s in srcs]),
    )

def _generate_upb_protos(ctx, generator, proto_info, proto_sources):
    if len(proto_sources) == 0:
        return GeneratedSrcsInfo(srcs = [], hdrs = [], thunks = [], includes = [])

    ext = "." + generator
    tool = getattr(ctx.executable, "_gen_" + generator)
    srcs = []
    if not (generator == "upb" and _is_google3):
        # TODO: The OSS build should also exclude this file for the upb generator,
        # as it is empty and unnecessary.  We only added it to make the OSS build happy on
        # Windows and macOS.
        srcs += [_generate_output_file(ctx, name, ext + ".c") for name in proto_sources]
    hdrs = [_generate_output_file(ctx, name, ext + ".h") for name in proto_sources]
    thunks = []
    if generator == "upb":
        thunks = [_generate_output_file(ctx, name, ext + ".thunks.c") for name in proto_sources]
    transitive_sets = proto_info.transitive_descriptor_sets.to_list()

    args = ctx.actions.args()
    args.use_param_file(param_file_arg = "@%s")
    args.set_param_file_format("multiline")

    args.add("--" + generator + "_out=" + _get_real_root(ctx, hdrs[0]))
    args.add("--plugin=protoc-gen-" + generator + "=" + tool.path)
    args.add("--descriptor_set_in=" + ctx.configuration.host_path_separator.join([f.path for f in transitive_sets]))
    args.add_all(proto_sources, map_each = _get_real_short_path)

    ctx.actions.run(
        inputs = depset(
            direct = [proto_info.direct_descriptor_set],
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        tools = [tool],
        outputs = srcs + hdrs,
        executable = ctx.executable._protoc,
        arguments = [args],
        progress_message = "Generating upb protos for :" + ctx.label.name,
        mnemonic = "GenUpbProtos",
    )
    if generator == "upb":
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
    return GeneratedSrcsInfo(
        srcs = srcs,
        hdrs = hdrs,
        thunks = thunks,
        includes = [_generate_include_path(proto_sources[0], hdrs[0], ext + ".h")],
    )

def _generate_name(ctx, generator, thunks = False):
    if thunks:
        return ctx.rule.attr.name + "." + generator + ".thunks"
    return ctx.rule.attr.name + "." + generator

def _get_dep_cc_infos(target, ctx, generator, cc_provider, dep_cc_provider):
    aspect_deps = getattr(ctx.attr, "_" + generator)
    rule_deps = ctx.rule.attr.deps
    dep_ccinfos = [dep[CcInfo] for dep in aspect_deps]
    dep_ccinfos += [dep[cc_provider].cc_info for dep in rule_deps]
    if dep_cc_provider:
        # This gives access to our direct sibling.  eg. foo.upb.h can #include "foo.upb_minitable.h"
        dep_ccinfos.append(target[dep_cc_provider].cc_info)

        # This gives access to imports.  eg. foo.upb.h can #include "import1.upb_minitable.h"
        # But not transitive imports, eg. foo.upb.h cannot #include "transitive_import1.upb_minitable.h"
        dep_ccinfos += [dep[dep_cc_provider].cc_info for dep in rule_deps]

    return dep_ccinfos

def _compile_upb_protos(ctx, files, generator, dep_ccinfos, cc_provider):
    cc_info = cc_library_func(
        ctx = ctx,
        name = _generate_name(ctx, generator),
        hdrs = files.hdrs,
        srcs = files.srcs,
        includes = files.includes,
        copts = ctx.attr._copts[UpbProtoLibraryCoptsInfo].copts,
        dep_ccinfos = dep_ccinfos,
    )

    if files.thunks:
        cc_info_with_thunks = cc_library_func(
            ctx = ctx,
            name = _generate_name(ctx, generator, files.thunks),
            hdrs = [],
            srcs = files.thunks,
            includes = files.includes,
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
            proto_info.direct_sources,
        )
        wrapped_cc_info = _compile_upb_protos(
            ctx,
            files,
            generator,
            dep_ccinfos,
            cc_provider,
        )

    hints = _get_hint_providers(ctx, generator) if provide_cc_shared_library_hints else []

    return hints + [
        file_provider(srcs = files),
        wrapped_cc_info,
    ]
