"""Definitions for internals of Kotlin proto libraries."""

load("//tools/build_defs/kotlin/release/rules:common.bzl", "common")
load(
    "//tools/build_defs/kotlin/release/rules:dep_checks_aspect.bzl",
    "KtForbiddenInfo",
    "dep_checks_aspect",
)
load(
    "//tools/build_defs/kotlin/release/toolchains/kotlin_jvm:toolchain.bzl",
    _toolchain = "toolchain",
)

def _get_real_short_path(file):
    """Returns the correct short path file name to be used by protoc."""
    short_path = file.short_path
    if short_path.startswith("../"):
        second_slash = short_path.index("/", 3)
        short_path = short_path[second_slash + 1:]

    virtual_imports = "_virtual_imports/"
    if virtual_imports in short_path:
        short_path = short_path.split(virtual_imports)[1].split("/", 1)[1]
    return short_path

def _kt_proto_extensions_impl(ctx):
    for dep in ctx.attr.proto_deps + ctx.attr.deps + ctx.attr.exports:
        if dep[KtForbiddenInfo].forbidden:
            fail("""Not allowed to depend on %s from Kotlin code, see go/kotlin/build-rules#restrictions:
           %s""" % (dep.label, "\n".join(dep[KtForbiddenInfo].forbidden)))

    transitive_set = depset(
        transitive =
            [dep[ProtoInfo].transitive_descriptor_sets for dep in ctx.attr.proto_deps],
    )
    proto_sources = []
    for dep in ctx.attr.proto_deps:
        for file in dep[ProtoInfo].direct_sources:
            proto_sources.append(_get_real_short_path(file))

    gen_src_dir = ctx.actions.declare_directory(ctx.label.name + "/ktproto")
    codegen_params = "lite:" if ctx.attr.variant == "lite" else ""

    protoc_args = ctx.actions.args()
    protoc_args.add("--kotlin_out=" + codegen_params + gen_src_dir.path)
    protoc_args.add_joined(
        transitive_set,
        join_with = ctx.configuration.host_path_separator,
        format_joined = "--descriptor_set_in=%s",
    )
    protoc_args.add_all(proto_sources)

    ctx.actions.run(
        inputs = depset(transitive = [transitive_set]),
        outputs = [gen_src_dir],
        executable = ctx.executable._protoc,
        arguments = [protoc_args],
        progress_message = "Generating kotlin proto extensions for " +
                           ", ".join([
                               str(dep.label)
                               for dep in ctx.attr.proto_deps
                           ]),
    )

    kt_toolchain = _toolchain.get(ctx)
    compiler_plugins = [
        kt_toolchain.marker_plugin,
        kt_toolchain.strictdeps_plugin,
    ]
    result = common.kt_jvm_library(
        ctx,
        srcs = [gen_src_dir],
        output = ctx.outputs.jar,
        deps = [dep[JavaInfo] for dep in ctx.attr.deps],
        exports = [dep[JavaInfo] for dep in ctx.attr.exports],
        kotlincopts = ["-Xopt-in=kotlin.RequiresOptIn", "-nowarn"],
        kt_toolchain = kt_toolchain,
        java_toolchain = ctx.attr._java_toolchain,
        compiler_plugins = compiler_plugins,
        android_compatible = "android" in ctx.attr.constraints,
        enforce_strict_deps = False,
    )
    java_info = java_common.add_constraints(
        result.java_info,
        constraints = ctx.attr.constraints,
    )

    transitive_runfiles = []
    for p in common.collect_providers(DefaultInfo, ctx.attr.deps + ctx.attr.exports):
        transitive_runfiles.append(p.data_runfiles.files)
        transitive_runfiles.append(p.default_runfiles.files)
    runfiles = ctx.runfiles(
        files = [ctx.outputs.jar],
        transitive_files = depset(transitive = transitive_runfiles),
        collect_default = True,
    )

    return [
        java_info,
        OutputGroupInfo(_validation = depset(result.validations)),
        DefaultInfo(
            files = depset([ctx.outputs.jar]),
            runfiles = runfiles,
        ),
        ProguardSpecProvider(depset(ctx.files._proguard_specs)),
    ]

kt_proto_library_helper = rule(
    doc = """
    This is a helper rule to combine the kotlin code generation work
    of kt_jvm_proto_library and kt_jvm_lite_proto_library.

    It generates Kotlin proto code with the protoc compiler using the
    --kotlin_out flag and compiles that Kotlin code. It requires as a dependency
    a proto_library and either a java_proto_library or java_lite_proto_library.
    """,
    attrs = dict(
        proto_deps = attr.label_list(
            providers = [ProtoInfo],
            aspects = [dep_checks_aspect],
        ),
        deps = attr.label_list(
            providers = [JavaInfo],
            aspects = [dep_checks_aspect],
        ),
        exports = attr.label_list(
            allow_rules = ["java_proto_library", "java_lite_proto_library"],
            aspects = [dep_checks_aspect],
        ),
        constraints = attr.string_list(
            doc = """Extra constraints imposed on this library, most commonly, 'android'. Constraints
                     typically indicate that the library may be used in binaries running outside of
                     Google production services. See go/be-java#java_library.constraints for details on
                     supported constraints and what they mean. Note that all Java and Kotlin `deps` are
                     required to carry compatible constraints.""",
        ),
        variant = attr.string(
            mandatory = True,
            values = ["lite", "full"],
            doc = """Indicator of which Kotlin proto runtime to use.""",
        ),
        _protoc = attr.label(
            default = Label("//net/proto2/compiler/public:protocol_compiler"),
            cfg = "host",
            executable = True,
        ),
        _java_toolchain = attr.label(
            default = Label(
                "//tools/jdk:current_java_toolchain",
            ),
        ),
        _coverage_instrumenter = attr.label(
            default = Label("//tools/build_defs/kotlin/release/rules:kt_coverage_instrumenter"),
            cfg = "host",
            executable = True,
        ),
        _coverage_runtime = attr.label(
            default = Label("//third_party/java/jacoco:blaze-agent"),
        ),
        _toolchain = attr.label(
            default = Label(
                "//tools/build_defs/kotlin/release/toolchains/kotlin_jvm:kt_jvm_toolchain_internal_DO_NOT_USE",
            ),
        ),
        _proguard_specs = attr.label_list(
            allow_files = [".pgcfg"],
            default = ["//java/com/google/protobuf/kotlin:kt_proto.pgcfg"],
        ),
    ),
    fragments = ["java"],
    outputs = dict(
        jar = "lib%{name}.ktproto.jar",
    ),
    provides = [JavaInfo],
    implementation = _kt_proto_extensions_impl,
    toolchains = [_toolchain.type],
)
