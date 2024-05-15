"""This file implements an experimental, do-not-use-kind of rust_proto_library.

Disclaimer: This project is experimental, under heavy development, and should not
be used yet."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@rules_proto//proto:defs.bzl", "ProtoInfo", "proto_common")

# buildifier: disable=bzl-visibility
load("@rules_rust//rust/private:providers.bzl", "CrateInfo", "DepInfo", "DepVariantInfo")

# buildifier: disable=bzl-visibility
load("@rules_rust//rust/private:rustc.bzl", "rustc_compile_action")
load("//bazel:upb_proto_library.bzl", "UpbWrappedCcInfo", "upb_proto_library_aspect")

visibility(["//rust/..."])

CrateMappingInfo = provider(
    doc = "Struct mapping crate name to the .proto import paths",
    fields = {
        "crate_name": "Crate name of the proto_library target",
        "import_paths": "Import path used in .proto files of dependants to import the .proto " +
                        "file of the current proto_library.",
    },
)
RustProtoInfo = provider(
    doc = "Rust protobuf provider info",
    fields = {
        "dep_variant_info": "DepVariantInfo for the compiled Rust gencode (also covers its " +
                            "transitive dependencies)",
        "crate_mapping": "depset(CrateMappingInfo) containing mappings of all transitive " +
                         "dependencies of the current proto_library.",
    },
)

def proto_rust_toolchain_label(is_upb):
    if is_upb:
        return "//rust:proto_rust_upb_toolchain"
    else:
        return "//rust:proto_rust_cpp_toolchain"

def _register_crate_mapping_write_action(name, actions, crate_mappings):
    """Registers an action that generates a crate mapping for a proto_library.

    Args:
        name: The name of the target being analyzed.
        actions: The context's actions object.
        crate_mappings: depset(CrateMappingInfo) to be rendered.
            This sequence should already have duplicates removed.

    Returns:
        The generated `File` with the crate mapping.
    """
    mapping_file = actions.declare_file(
        "{}.rust_crate_mapping".format(name),
    )
    content = actions.args()
    content.set_param_file_format("multiline")
    content.add_all(crate_mappings, map_each = _render_text_crate_mapping)
    actions.write(content = content, output = mapping_file)

    return mapping_file

def _render_text_crate_mapping(mapping):
    """Renders the mapping to an easily parseable file for a crate mapping.

    Args:
        mapping (CrateMappingInfo): A single crate mapping.

    Returns:
        A string containing the crate mapping for the target in simple format:
            <crate_name>\n
            <number of lines to follow>\n
            <one import path per line>\n
    """
    crate_name = mapping.crate_name

    # proto_library targets may contain '-', but rust crates don't.
    crate_name = crate_name.replace("-", "_")
    import_paths = mapping.import_paths
    return "\n".join(([crate_name, str(len(import_paths))] + list(import_paths)))

def _generate_rust_gencode(
        ctx,
        proto_info,
        proto_lang_toolchain,
        crate_mapping,
        is_upb):
    """Generates Rust gencode

        This function uses proto_common APIs and a ProtoLangToolchain to register an action
        that invokes protoc with the right flags.
    Args:
        ctx (RuleContext): current rule context
        proto_info (ProtoInfo): ProtoInfo of the proto_library target for which we are generating
                    gencode
        proto_lang_toolchain (ProtoLangToolchainInfo): proto lang toolchain for Rust
        crate_mapping (File): File containing the mapping from .proto file import path to its
                      corresponding containing Rust crate name.
        is_upb (Bool): True when generating gencode for UPB, False otherwise.
    Returns:
        rs_outputs (([File], [File]): tuple(generated Rust files, generated C++ thunks).
    """
    actions = ctx.actions
    rs_outputs = proto_common.declare_generated_files(
        actions = actions,
        proto_info = proto_info,
        extension = ".{}.pb.rs".format("u" if is_upb else "c"),
    )
    if is_upb:
        cc_outputs = []
    else:
        cc_outputs = proto_common.declare_generated_files(
            actions = actions,
            proto_info = proto_info,
            extension = ".pb.thunks.cc",
        )
    additional_args = ctx.actions.args()

    additional_args.add(
        "--rust_opt=experimental-codegen=enabled,kernel={},bazel_crate_mapping={}".format(
            "upb" if is_upb else "cpp",
            crate_mapping.path,
        ),
    )

    proto_common.compile(
        actions = ctx.actions,
        proto_info = proto_info,
        additional_inputs = depset(direct = [crate_mapping]),
        additional_args = additional_args,
        generated_files = rs_outputs + cc_outputs,
        proto_lang_toolchain_info = proto_lang_toolchain,
        plugin_output = ctx.bin_dir.path,
    )
    return (rs_outputs, cc_outputs)

def _get_crate_info(providers):
    for provider in providers:
        if hasattr(provider, "name"):
            return provider
    fail("Couldn't find a CrateInfo in the list of providers")

def _get_dep_info(providers):
    for provider in providers:
        if hasattr(provider, "direct_crates"):
            return provider
    fail("Couldn't find a DepInfo in the list of providers")

def _get_cc_info(providers):
    for provider in providers:
        if hasattr(provider, "linking_context"):
            return provider
    fail("Couldn't find a CcInfo in the list of providers")

def _compile_cc(
        ctx,
        attr,
        cc_toolchain,
        feature_configuration,
        src,
        cc_infos):
    """Compiles a C++ source file.

    Args:
      ctx: The rule context.
      attr: The current rule's attributes.
      cc_toolchain: A cc_toolchain.
      feature_configuration: A feature configuration.
      src: The source file to be compiled.
      cc_infos: List[CcInfo]: A list of CcInfo dependencies.

    Returns:
      A CcInfo provider.
    """
    cc_info = cc_common.merge_cc_infos(direct_cc_infos = cc_infos)

    (compilation_context, compilation_outputs) = cc_common.compile(
        name = src.basename,
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        srcs = [src],
        user_compile_flags = attr.copts if hasattr(attr, "copts") else [],
        compilation_contexts = [cc_info.compilation_context],
    )

    (linking_context, _) = cc_common.create_linking_context_from_compilation_outputs(
        name = src.basename,
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        compilation_outputs = compilation_outputs,
        linking_contexts = [cc_info.linking_context],
    )

    return CcInfo(
        compilation_context = compilation_context,
        linking_context = linking_context,
    )

def _compile_rust(ctx, attr, src, extra_srcs, deps):
    """Compiles a Rust source file.

    Eventually this function could be upstreamed into rules_rust and be made present in rust_common.

    Args:
      ctx (RuleContext): The rule context.
      attr (Attrs): The current rule's attributes (`ctx.attr` for rules, `ctx.rule.attr` for aspects)
      src (File): The crate root source file to be compiled.
      extra_srcs ([File]): Additional source files to include in the crate.
      deps (List[DepVariantInfo]): A list of dependencies needed.

    Returns:
      A DepVariantInfo provider.
    """
    toolchain = ctx.toolchains["@rules_rust//rust:toolchain_type"]
    output_hash = repr(hash(src.path))

    # TODO: Use the import! macro once available
    crate_name = ctx.label.name.replace("-", "_")

    lib_name = "{prefix}{name}-{lib_hash}{extension}".format(
        prefix = "lib",
        name = crate_name,
        lib_hash = output_hash,
        extension = ".rlib",
    )

    rmeta_name = "{prefix}{name}-{lib_hash}{extension}".format(
        prefix = "lib",
        name = crate_name,
        lib_hash = output_hash,
        extension = ".rmeta",
    )

    lib = ctx.actions.declare_file(lib_name)
    rmeta = ctx.actions.declare_file(rmeta_name)

    # TODO: Use higher level rules_rust API once available.
    providers = rustc_compile_action(
        ctx = ctx,
        attr = attr,
        toolchain = toolchain,
        crate_info_dict = dict(
            name = crate_name,
            type = "rlib",
            root = src,
            srcs = depset([src] + extra_srcs),
            deps = depset(deps),
            proc_macro_deps = depset([]),
            aliases = {},
            output = lib,
            metadata = rmeta,
            edition = "2021",
            is_test = False,
            rustc_env = {},
            compile_data = depset([]),
            compile_data_targets = depset([]),
            owner = ctx.label,
        ),
        # Needed to make transitive public imports not violate layering.
        force_all_deps_direct = True,
        output_hash = output_hash,
    )

    return DepVariantInfo(
        crate_info = _get_crate_info(providers),
        dep_info = _get_dep_info(providers),
        cc_info = _get_cc_info(providers),
        build_info = None,
    )

def _rust_upb_proto_aspect_impl(target, ctx):
    """Implements the Rust protobuf aspect logic for UPB kernel."""
    return _rust_proto_aspect_common(target, ctx, is_upb = True)

def _rust_cc_proto_aspect_impl(target, ctx):
    """Implements the Rust protobuf aspect logic for C++ kernel."""
    return _rust_proto_aspect_common(target, ctx, is_upb = False)

def get_import_path(f):
    if hasattr(proto_common, "get_import_path"):
        return proto_common.get_import_path(f)
    else:
        return f.path

def _rust_proto_aspect_common(target, ctx, is_upb):
    if RustProtoInfo in target:
        return []

    proto_lang_toolchain = ctx.attr._proto_lang_toolchain[proto_common.ProtoLangToolchainInfo]
    cc_toolchain = find_cpp_toolchain(ctx)

    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    proto_srcs = getattr(ctx.rule.files, "srcs", [])
    proto_deps = getattr(ctx.rule.attr, "deps", [])
    transitive_crate_mappings = []
    for dep in proto_deps:
        rust_proto_info = dep[RustProtoInfo]
        transitive_crate_mappings.append(rust_proto_info.crate_mapping)

    mapping_for_current_target = depset(transitive = transitive_crate_mappings)
    crate_mapping_file = _register_crate_mapping_write_action(
        target.label.name,
        ctx.actions,
        mapping_for_current_target,
    )

    (gencode, thunks) = _generate_rust_gencode(
        ctx,
        target[ProtoInfo],
        proto_lang_toolchain,
        crate_mapping_file,
        is_upb,
    )

    if is_upb:
        thunks_cc_info = target[UpbWrappedCcInfo].cc_info_with_thunks
    else:
        dep_cc_infos = []
        for dep in proto_deps:
            dep_cc_infos.append(dep[CcInfo])

        thunks_cc_info = cc_common.merge_cc_infos(cc_infos = [_compile_cc(
            feature_configuration = feature_configuration,
            src = thunk,
            ctx = ctx,
            attr = attr,
            cc_toolchain = cc_toolchain,
            cc_infos = [target[CcInfo], ctx.attr._cpp_thunks_deps[CcInfo]] + dep_cc_infos,
        ) for thunk in thunks])

    runtime = proto_lang_toolchain.runtime
    dep_variant_info_for_runtime = DepVariantInfo(
        crate_info = runtime[CrateInfo] if CrateInfo in runtime else None,
        dep_info = runtime[DepInfo] if DepInfo in runtime else None,
        cc_info = runtime[CcInfo] if CcInfo in runtime else None,
        build_info = None,
    )
    dep_variant_info_for_native_gencode = DepVariantInfo(cc_info = thunks_cc_info)

    dep_variant_info = _compile_rust(
        ctx = ctx,
        attr = ctx.rule.attr,
        src = gencode[0],
        extra_srcs = gencode[1:],
        deps = [dep_variant_info_for_runtime, dep_variant_info_for_native_gencode] + (
            [d[RustProtoInfo].dep_variant_info for d in proto_deps]
        ),
    )
    return [RustProtoInfo(
        dep_variant_info = dep_variant_info,
        crate_mapping = depset(
            direct = [CrateMappingInfo(
                crate_name = target.label.name,
                import_paths = tuple([get_import_path(f) for f in proto_srcs]),
            )],
            transitive = transitive_crate_mappings,
        ),
    )]

def _make_proto_library_aspect(is_upb):
    return aspect(
        implementation = (_rust_upb_proto_aspect_impl if is_upb else _rust_cc_proto_aspect_impl),
        attr_aspects = ["deps"],
        requires = ([upb_proto_library_aspect] if is_upb else [cc_proto_aspect]),
        attrs = {
            "_cc_toolchain": attr.label(
                doc = (
                    "In order to use find_cc_toolchain, your rule has to depend " +
                    "on C++ toolchain. See `@rules_cc//cc:find_cc_toolchain.bzl` " +
                    "docs for details."
                ),
                default = Label("@bazel_tools//tools/cpp:current_cc_toolchain"),
            ),
            "_collect_cc_coverage": attr.label(
                default = Label("@rules_rust//util:collect_coverage"),
                executable = True,
                cfg = "exec",
            ),
            "_cpp_thunks_deps": attr.label(
                default = Label("//rust/cpp_kernel:cpp_api"),
            ),
            "_error_format": attr.label(
                default = Label("@rules_rust//:error_format"),
            ),
            "_extra_exec_rustc_flag": attr.label(
                default = Label("@rules_rust//:extra_exec_rustc_flag"),
            ),
            "_extra_exec_rustc_flags": attr.label(
                default = Label("@rules_rust//:extra_exec_rustc_flags"),
            ),
            "_extra_rustc_flag": attr.label(
                default = Label("@rules_rust//:extra_rustc_flag"),
            ),
            "_extra_rustc_flags": attr.label(
                default = Label("@rules_rust//:extra_rustc_flags"),
            ),
            "_process_wrapper": attr.label(
                doc = "A process wrapper for running rustc on all platforms.",
                default = Label("@rules_rust//util/process_wrapper"),
                executable = True,
                allow_single_file = True,
                cfg = "exec",
            ),
            "_proto_lang_toolchain": attr.label(
                default = Label(proto_rust_toolchain_label(is_upb)),
            ),
        },
        fragments = ["cpp"],
        toolchains = [
            "@rules_rust//rust:toolchain_type",
            "@bazel_tools//tools/cpp:toolchain_type",
        ],
    )

rust_upb_proto_library_aspect = _make_proto_library_aspect(is_upb = True)
rust_cc_proto_library_aspect = _make_proto_library_aspect(is_upb = False)
