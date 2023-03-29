"""This file implements an experimental, do-not-use-kind of rust_proto_library.

Disclaimer: This project is experimental, under heavy development, and should not
be used yet."""

# buildifier: disable=bzl-visibility
load("@rules_rust//rust/private:providers.bzl", "CrateInfo", "DepInfo", "DepVariantInfo")

# buildifier: disable=bzl-visibility
load("@rules_rust//rust/private:rustc.bzl", "rustc_compile_action")
load("@rules_rust//rust:defs.bzl", "rust_common")
load("@upb//bazel:upb_proto_library.bzl", "UpbWrappedCcInfo", "upb_proto_library_aspect")

proto_common = proto_common_do_not_use

RustProtoInfo = provider(
    doc = "Rust protobuf provider info",
    fields = {
        "dep_variant_info": "DepVariantInfo for the compiled Rust gencode (also covers its " +
                            "transitive dependencies)",
    },
)

def _generate_rust_gencode(
        ctx,
        proto_info,
        proto_lang_toolchain,
        ext):
    """Generates Rust gencode

        This function uses proto_common APIs and a ProtoLangToolchain to register an action
        that invokes protoc with the right flags.
    Args:
        ctx (RuleContext): current rule context
        proto_info (ProtoInfo): ProtoInfo of the proto_library target for which we are generating
                    gencode
        proto_lang_toolchain (ProtoLangToolchainInfo): proto lang toolchain for Rust
        ext: the file extension to use for generated files
    Returns:
        rs_outputs ([File]): generated Rust source files
    """
    actions = ctx.actions
    rs_outputs = proto_common.declare_generated_files(
        actions = actions,
        proto_info = proto_info,
        extension = ext,
    )

    proto_common.compile(
        actions = ctx.actions,
        proto_info = proto_info,
        generated_files = rs_outputs,
        proto_lang_toolchain_info = proto_lang_toolchain,
        plugin_output = ctx.bin_dir.path,
    )
    return rs_outputs

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
    toolchain = ctx.toolchains["@rules_rust//rust:toolchain"]
    output_hash = repr(hash(src.path))

    # TODO(b/270124215): Use the import! macro once available
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

    providers = rustc_compile_action(
        ctx = ctx,
        attr = attr,
        toolchain = toolchain,
        crate_info = rust_common.create_crate_info(
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
        output_hash = output_hash,
    )

    return DepVariantInfo(
        crate_info = _get_crate_info(providers),
        dep_info = _get_dep_info(providers),
        cc_info = _get_cc_info(providers),
        build_info = None,
    )

def _is_cc_proto_library(rule):
    """Detects if the current rule is a cc_proto_library."""
    return rule.kind == "cc_proto_library"

def _rust_upb_proto_aspect_impl(target, ctx):
    """Implements the Rust protobuf aspect logic for UPB kernel."""
    return _rust_proto_aspect_common(target, ctx, is_upb = True)

def _rust_cc_proto_aspect_impl(target, ctx):
    """Implements the Rust protobuf aspect logic for C++ kernel."""
    return _rust_proto_aspect_common(target, ctx, is_upb = False)

def _rust_proto_aspect_common(target, ctx, is_upb):
    if RustProtoInfo in target:
        return []

    if _is_cc_proto_library(ctx.rule):
        # This is cc_proto_library, but we need the RustProtoInfo provider of the proto_library that
        # this aspect provides. Luckily this aspect has already been attached on the proto_library
        # so we can just read the provider.
        return [ctx.rule.attr.deps[0][RustProtoInfo]]

    proto_lang_toolchain = ctx.attr._proto_lang_toolchain[proto_common.ProtoLangToolchainInfo]

    gencode = _generate_rust_gencode(ctx, target[ProtoInfo], proto_lang_toolchain, (".u.pb.rs" if is_upb else ".c.pb.rs"))

    runtime = proto_lang_toolchain.runtime
    dep_variant_info_for_runtime = DepVariantInfo(
        crate_info = runtime[CrateInfo] if CrateInfo in runtime else None,
        dep_info = runtime[DepInfo] if DepInfo in runtime else None,
        cc_info = runtime[CcInfo] if CcInfo in runtime else None,
        build_info = None,
    )
    dep_variant_info_for_native_gencode = DepVariantInfo(
        cc_info = target[UpbWrappedCcInfo].cc_info_with_thunks if is_upb else target[CcInfo],
    )

    proto_dep = getattr(ctx.rule.attr, "deps", [])
    dep_variant_info = _compile_rust(
        ctx = ctx,
        attr = ctx.rule.attr,
        src = gencode[0],
        extra_srcs = gencode[1:],
        deps = [dep_variant_info_for_runtime, dep_variant_info_for_native_gencode] + (
            [proto_dep[0][RustProtoInfo].dep_variant_info] if proto_dep else []
        ),
    )
    return [RustProtoInfo(dep_variant_info = dep_variant_info)]

def _make_proto_library_aspect(is_upb):
    return aspect(
        implementation = (_rust_upb_proto_aspect_impl if is_upb else _rust_cc_proto_aspect_impl),
        attr_aspects = ["deps"],
        # Since we can reference upb_proto_library_aspect by name, we can just ask Bazel to attach
        # it here.
        requires = ([upb_proto_library_aspect] if is_upb else []),
        required_aspect_providers = ([] if is_upb else [CcInfo]),
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
                default = Label(("//rust:proto_rust_upb_toolchain" if is_upb else "//rust:proto_rust_cpp_toolchain")),
            ),
        },
        fragments = ["cpp"],
        host_fragments = ["cpp"],
        toolchains = [
            str(Label("@rules_rust//rust:toolchain")),
            "@bazel_tools//tools/cpp:toolchain_type",
        ],
        incompatible_use_toolchain_transition = True,
    )

rust_upb_proto_library_aspect = _make_proto_library_aspect(is_upb = True)
rust_cc_proto_library_aspect = _make_proto_library_aspect(is_upb = False)
