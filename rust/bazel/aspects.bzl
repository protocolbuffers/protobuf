# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

"""This file implements rust_proto_library aspect."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_rust//:version.bzl", RUST_VERSION = "VERSION")

# buildifier: disable=bzl-visibility
load("@rules_rust//rust/private:providers.bzl", "CrateInfo", "DepInfo", "DepVariantInfo")

# buildifier: disable=bzl-visibility
load("@rules_rust//rust/private:rustc.bzl", "rustc_compile_action")
load("//bazel/common:proto_common.bzl", "proto_common")
load("//bazel/common:proto_info.bzl", "ProtoInfo")
load("//bazel/private:cc_proto_aspect.bzl", "cc_proto_aspect")
load(
    "encode_raw_string_as_crate_name.bzl",
    "encode_raw_string_as_crate_name",
)

visibility([
    "//net/proto2/compiler/stubby/...",
    "//rust/...",
    "//third_party/crubit/rs_bindings_from_cc/...",
])

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
        "dep_variant_infos": "List(DepVariantInfo): infos for the compiled Rust gencode. \
            When the target is a regular proto library, this contains a single element -- the info for the top-level generated code. \
            When the target is a srcsless proto library, this contains the infos of its dependencies.",
        "exports_dep_variant_infos": "List(DepVariantInfo): Transitive infos from targets from the proto_library.exports attribute. \
            When the target is a srcsless proto library, this contains the exports infos from all of its dependencies. \
            This is a list instead of a depset, since we pass them as direct dependencies when compiling rust code. \
            We assume the proto exports feature is not widely used in a way where this will lead to unacceptable analysis-time overhead.",
        "crate_mapping": "depset(CrateMappingInfo) containing mappings of all transitive " +
                         "dependencies of the current proto_library.",
    },
)

def _version_parts(version):
    major, minor = version.split(".")[0:2]
    return (int(major), int(minor))

def _rust_version_ge(version):
    """Checks if the rust version as at least the given major.minor version."""
    return _version_parts(RUST_VERSION) >= _version_parts(version)

def label_to_crate_name(ctx, label, toolchain):
    return label.name.replace("-", "_")

def encode_label_as_crate_name(label):
    return encode_raw_string_as_crate_name(str(label))

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
    import_paths = mapping.import_paths
    return "\n".join(([crate_name, str(len(import_paths))] + list(import_paths)))

def _generate_rust_gencode(
        ctx,
        proto_info,
        proto_lang_toolchain,
        crate_mapping,
        feature_configuration,
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
        feature_configuration (FeatureConfiguration): A feature configuration.
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

    # We emit one 'entry point' file which is based on the target name.
    # Unfortunately, target names may contain slashes which we have to avoid.
    safe_target_name = ctx.label.name.replace("/", "__")
    entry_point_rs_output = actions.declare_file(
        "{}.generated.{}.rs".format(safe_target_name, "u" if is_upb else "c"),
        sibling = proto_info.direct_sources[0],
    )

    if is_upb:
        cc_outputs = []
    else:
        cc_outputs = proto_common.declare_generated_files(
            actions = actions,
            proto_info = proto_info,
            extension = ".pb.thunks.cc",
        )
    forced_lite_runtime = cc_common.is_enabled(feature_configuration = feature_configuration, feature_name = "proto_force_lite_runtime")

    additional_args = ctx.actions.args()

    additional_args.add(
        "--rust_opt=experimental-codegen=enabled,kernel={},crate_mapping={},generated_entry_point_rs_file_name={},forced_lite_runtime={}".format(
            "upb" if is_upb else "cpp",
            crate_mapping.path,
            entry_point_rs_output.basename,
            "true" if forced_lite_runtime else "false",
        ),
    )

    proto_common.compile(
        actions = ctx.actions,
        proto_info = proto_info,
        additional_inputs = depset(direct = [crate_mapping]),
        additional_args = additional_args,
        generated_files = [entry_point_rs_output] + rs_outputs + cc_outputs,
        proto_lang_toolchain_info = proto_lang_toolchain,
    )
    return (entry_point_rs_output, rs_outputs, cc_outputs)

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
        name = src.short_path,
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        srcs = [src],
        user_compile_flags = attr.copts if hasattr(attr, "copts") else [],
        compilation_contexts = [cc_info.compilation_context],
    )

    (linking_context, _) = cc_common.create_linking_context_from_compilation_outputs(
        name = src.short_path,
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

def _compile_rust(ctx, attr, src, extra_srcs, deps, aliases, runtime):
    """Compiles a Rust source file.

    Eventually this function could be upstreamed into rules_rust and be made present in rust_common.

    Args:
      ctx (RuleContext): The rule context.
      attr (Attrs): The current rule's attributes (`ctx.attr` for rules, `ctx.rule.attr` for aspects)
      src (File): The crate root source file to be compiled.
      extra_srcs ([File]): Additional source files to include in the crate.
      deps (List[DepVariantInfo]): A list of dependencies needed.
      aliases (dict[Target, str]): A mapping from dependency target to its crate name.
      runtime: The protobuf runtime target.

    Returns:
      A DepVariantInfo provider.
    """
    toolchain = ctx.toolchains["@rules_rust//rust:toolchain_type"]
    output_hash = repr(hash(src.path))

    crate_name = label_to_crate_name(ctx, ctx.label, toolchain)

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

    if _rust_version_ge("0.66"):
        deps = deps
        proc_macro_deps = []
    else:
        deps = depset(deps)
        proc_macro_deps = depset()

    if _rust_version_ge("0.67"):
        srcs = [src] + extra_srcs
    else:
        srcs = depset([src] + extra_srcs)

    # TODO: Use higher level rules_rust API once available.
    providers = rustc_compile_action(
        ctx = ctx,
        attr = attr,
        toolchain = toolchain,
        crate_info_dict = dict(
            name = crate_name,
            type = "rlib",
            root = src,
            srcs = srcs,
            deps = deps,
            proc_macro_deps = proc_macro_deps,
            # Make "protobuf" into an alias for the runtime. This allows the
            # generated code to use a consistent name, even though the actual
            # name of the runtime crate varies depending on the protobuf kernel
            # and build system.
            aliases = {runtime: "protobuf"} | aliases,
            output = lib,
            metadata = rmeta,
            edition = "2024",
            is_test = False,
            rustc_env = {},
            compile_data = depset([]),
            compile_data_targets = depset([]),
            owner = ctx.label,
        ),
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

    if ProtoInfo not in target:
        return []

    proto_srcs = target[ProtoInfo].direct_sources
    proto_deps = getattr(ctx.rule.attr, "deps", [])
    transitive_crate_mappings = []
    for dep in proto_deps:
        rust_proto_info = dep[RustProtoInfo]
        transitive_crate_mappings.append(rust_proto_info.crate_mapping)

    dep_variant_infos = []

    # Infos of exports of dependencies.
    dep_exports_dep_variant_infos = []

    for dep in proto_deps:
        dep_variant_infos += dep[RustProtoInfo].dep_variant_infos
        dep_exports_dep_variant_infos += dep[RustProtoInfo].exports_dep_variant_infos

    # If there are no srcs, then this is an alias library (which in Rust acts as a middle
    # library in a dependency chain). Don't generate any Rust code for it, but do propagate the
    # crate mappings.
    if not proto_srcs:
        return [RustProtoInfo(
            dep_variant_infos = dep_variant_infos,
            exports_dep_variant_infos = dep_exports_dep_variant_infos,
            crate_mapping = depset(transitive = transitive_crate_mappings),
        )]

    # Add the infos from dependencies' exports, as they are needed to compile the
    # generated code of this target.
    dep_variant_infos += dep_exports_dep_variant_infos

    # Exports of this target are the directly and transitively exported
    # dependencies.
    exported_proto_deps = getattr(ctx.rule.attr, "exports", [])
    exports_dep_variant_infos = []
    for d in exported_proto_deps:
        exports_dep_variant_infos.extend(d[RustProtoInfo].dep_variant_infos)
        exports_dep_variant_infos.extend(d[RustProtoInfo].exports_dep_variant_infos)

    proto_lang_toolchain = ctx.attr._proto_lang_toolchain[proto_common.ProtoLangToolchainInfo]
    cc_toolchain = find_cpp_toolchain(ctx)
    toolchain = ctx.toolchains["@rules_rust//rust:toolchain_type"]

    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features + ["module_maps"],
    )

    mapping_for_current_target = depset(transitive = transitive_crate_mappings)
    crate_mapping_file = _register_crate_mapping_write_action(
        target.label.name,
        ctx.actions,
        mapping_for_current_target,
    )

    (entry_point_rs_output, rs_gencode, cc_thunks_gencode) = _generate_rust_gencode(
        ctx,
        target[ProtoInfo],
        proto_lang_toolchain,
        crate_mapping_file,
        feature_configuration,
        is_upb,
    )

    dep_variant_info_for_native_gencode = []
    if not is_upb:
        dep_cc_infos = []
        for dep in proto_deps:
            dep_cc_infos.append(dep[CcInfo])

        thunks_cc_info = cc_common.merge_cc_infos(cc_infos = [_compile_cc(
            feature_configuration = feature_configuration,
            src = thunk,
            ctx = ctx,
            attr = attr,
            cc_toolchain = cc_toolchain,
            cc_infos = [target[CcInfo]] + [dep[CcInfo] for dep in ctx.attr._cpp_thunks_deps] + dep_cc_infos,
        ) for thunk in cc_thunks_gencode])

        dep_variant_info_for_native_gencode = [DepVariantInfo(
            cc_info = thunks_cc_info,
            dep_info = None,
            build_info = None,
            crate_info = None,
        )]

    runtime = proto_lang_toolchain.runtime
    dep_variant_info_for_runtime = DepVariantInfo(
        crate_info = runtime[CrateInfo] if CrateInfo in runtime else None,
        dep_info = runtime[DepInfo] if DepInfo in runtime else None,
        cc_info = runtime[CcInfo] if CcInfo in runtime else None,
        build_info = None,
    )

    extra_dep_variant_infos = [
        DepVariantInfo(
            crate_info = dep[CrateInfo] if CrateInfo in dep else None,
            dep_info = dep[DepInfo] if DepInfo in dep else None,
            cc_info = dep[CcInfo] if CcInfo in dep else None,
            build_info = None,
        )
        for dep in ctx.attr._extra_deps
    ]

    aliases = {}

    for d in dep_variant_infos:
        label = Label(d.crate_info.owner)
        target = struct(label = label)
        qualified_name = encode_label_as_crate_name(label)
        aliases[target] = qualified_name

    deps = ([dep_variant_info_for_runtime] +
            dep_variant_info_for_native_gencode +
            dep_variant_infos +
            extra_dep_variant_infos +
            exports_dep_variant_infos)

    dep_variant_info = _compile_rust(
        ctx = ctx,
        attr = ctx.rule.attr,
        src = entry_point_rs_output,
        extra_srcs = rs_gencode,
        deps = deps,
        aliases = aliases,
        runtime = runtime,
    )
    return [RustProtoInfo(
        dep_variant_infos = [dep_variant_info],
        exports_dep_variant_infos = exports_dep_variant_infos,
        crate_mapping = depset(
            direct = [CrateMappingInfo(
                crate_name = encode_label_as_crate_name(ctx.label),
                import_paths = tuple([get_import_path(f) for f in proto_srcs]),
            )],
            transitive = transitive_crate_mappings,
        ),
    )]

def _make_proto_library_aspect(is_upb):
    return aspect(
        implementation = (_rust_upb_proto_aspect_impl if is_upb else _rust_cc_proto_aspect_impl),
        attr_aspects = ["deps", "exports"],
        requires = ([] if is_upb else [cc_proto_aspect]),
        attrs = {
            "_collect_cc_coverage": attr.label(
                default = Label("@rules_rust//util:collect_coverage"),
                executable = True,
                cfg = "exec",
            ),
            "_cpp_thunks_deps": attr.label_list(
                default = [
                    Label("//rust/cpp_kernel:cpp_api"),
                    Label("//src/google/protobuf"),
                    Label("//src/google/protobuf:protobuf_lite"),
                ],
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
            "_extra_deps": attr.label_list(
                default = [
                ],
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
