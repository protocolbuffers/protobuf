"""Implementation of the aspect that powers the upb_*_proto_library() rules."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load(":upb_proto_library_internal/cc_library_func.bzl", "cc_library_func")
load(":upb_proto_library_internal/copts.bzl", "UpbProtoLibraryCoptsInfo")

_is_google3 = False

GeneratedSrcsInfo = provider(
    "Provides generated headers and sources",
    fields = {
        "srcs": "list of srcs",
        "hdrs": "list of hdrs",
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
    )

def _get_implicit_weak_field_sources(ctx, proto_info):
    # Creating one .cc file for each Message in a proto allows the linker to be more aggressive
    # about removing unused classes. However, since the number of outputs won't be known at Blaze
    # analysis time, all of the generated source files are put in a directory and a TreeArtifact is
    # used to represent them.
    proto_artifacts = []
    for proto_source in proto_info.direct_sources:
        # We can have slashes in the target name. For example, proto_source can be:
        # dir/a.proto. However proto_source.basename will return a.proto, when in reality
        # the BUILD file declares it as dir/a.proto, because target name contains a slash.
        # There is no good workaround for this.
        # I am using ctx.label.package to check if the name of the target contains slash or not.
        # This is similar to what declare_directory does.
        if not proto_source.short_path.startswith(ctx.label.package):
            fail("This should never happen, proto source {} path does not start with {}.".format(
                proto_source.short_path,
                ctx.label.package,
            ))
        proto_source_name = proto_source.short_path[len(ctx.label.package) + 1:]
        last_dot = proto_source_name.rfind(".")
        if last_dot != -1:
            proto_source_name = proto_source_name[:last_dot]
        proto_artifacts.append(ctx.actions.declare_directory(proto_source_name + ".upb_weak_minitables"))

    return proto_artifacts

def _get_feature_configuration(ctx, cc_toolchain, proto_info):
    requested_features = list(ctx.features)

    # Disable the whole-archive behavior for protobuf generated code when the
    # proto_one_output_per_message feature is enabled.
    requested_features.append("disable_whole_archive_for_static_lib_if_proto_one_output_per_message")
    unsupported_features = list(ctx.disabled_features)
    if len(proto_info.direct_sources) != 0:
        requested_features.append("header_modules")
    else:
        unsupported_features.append("header_modules")
    return cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = requested_features,
        unsupported_features = unsupported_features,
    )

def _generate_srcs_list(ctx, generator, proto_info):
    if len(proto_info.direct_sources) == 0:
        return GeneratedSrcsInfo(srcs = [], hdrs = [], includes = [])

    ext = "." + generator
    srcs = []
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

    return GeneratedSrcsInfo(
        srcs = srcs,
        hdrs = hdrs,
    )

def _generate_upb_protos(ctx, generator, proto_info, feature_configuration):
    implicit_weak = generator == "upb_minitable" and cc_common.is_enabled(
        feature_configuration = feature_configuration,
        feature_name = "proto_one_output_per_message",
    )

    srcs = _generate_srcs_list(ctx, generator, proto_info)
    additional_args = ctx.actions.args()

    if implicit_weak:
        srcs.srcs.extend(_get_implicit_weak_field_sources(ctx, proto_info))
        additional_args.add("--upb_minitable_opt=one_output_per_message")

    proto_common.compile(
        actions = ctx.actions,
        proto_info = proto_info,
        proto_lang_toolchain_info = _get_lang_toolchain(ctx, generator),
        generated_files = srcs.srcs + srcs.hdrs,
        experimental_exec_group = "proto_compiler",
        additional_args = additional_args,
    )

    return srcs

def _generate_name(ctx, generator):
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
        that we depend on. The aspect will be able to include the header files from this provider.
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
        # https://bazel.build/versions/6.4.0/reference/be/protocol-buffer#proto_library.srcs
        files = _merge_generated_srcs([dep[file_provider].srcs for dep in ctx.rule.attr.deps])
        wrapped_cc_info = cc_provider(
            cc_info = cc_common.merge_cc_infos(direct_cc_infos = dep_ccinfos),
        )
    else:
        proto_info = target[ProtoInfo]
        cc_toolchain = find_cpp_toolchain(ctx)
        feature_configuration = _get_feature_configuration(ctx, cc_toolchain, proto_info)
        files = _generate_upb_protos(
            ctx,
            generator,
            proto_info,
            feature_configuration,
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
