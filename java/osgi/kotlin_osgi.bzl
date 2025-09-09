""" Custom rule to generate OSGi Manifest for Kotlin """

load("@rules_java//java:defs.bzl", "JavaInfo")
load("@rules_kotlin//kotlin:jvm.bzl", "kt_jvm_library")

def osgi_kt_jvm_library(
        name,
        automatic_module_name,
        bundle_description,
        bundle_doc_url,
        bundle_license,
        bundle_name,
        bundle_symbolic_name,
        bundle_version,
        bundle_additional_imports = [],
        bundle_additional_exports = [],
        deps = [],
        exports = [],
        exported_plugins = [],
        neverlink = False,
        runtime_deps = [],
        visibility = [],
        **kwargs):
    """Extends `kt_jvm_library` to add OSGi headers to the MANIFEST.MF using bndlib

    This macro should be usable as a drop-in replacement for kt_jvm_library.

    The additional arguments are given the bndlib tool to generate an OSGi-compliant manifest file.
    See [bnd documentation](https://bnd.bndtools.org/chapters/110-introduction.html)

    Args:
        name: (required) A unique name for this target.
        automatic_module_name: (required) The Automatic-Module-Name header that represents
            the name of the module when this bundle is used as an automatic
            module.
        bundle_description: (required) The Bundle-Description header defines a short
            description of this bundle.
        bundle_doc_url: (required) The Bundle-DocURL headers must contain a URL pointing
            to documentation about this bundle.
        bundle_license: (required) The Bundle-License header provides an optional machine
            readable form of license information.
        bundle_name: (required) The Bundle-Name header defines a readable name for this
            bundle. This should be a short, human-readable name that can
            contain spaces.
        bundle_symbolic_name: (required) The Bundle-SymbolicName header specifies a
            non-localizable name for this bundle. The bundle symbolic name
            together with a version must identify a unique bundle though it can
            be installed multiple times in a framework. The bundle symbolic
            name should be based on the reverse domain name convention.
        bundle_version: (required) The Bundle-Version header specifies the version string
            for this bundle. The version string is expected to follow semantic
            versioning conventions MAJOR.MINOR.PATCH[.BUILD]
        bundle_additional_exports: The Export-Package header contains a
            declaration of exported packages. These are additional export
            package statements to be added before the default wildcard export
            "*;version={$Bundle-Version}".
        bundle_additional_imports: The Import-Package header declares the
            imported packages for this bundle. These are additional import
            package statements to be added before the default wildcard import
            "*".
        deps: The list of libraries to link into this library. See general
            comments about deps at Typical attributes defined by most build
            rules. The jars built by java_library rules listed in deps will be
            on the compile-time classpath of this rule. Furthermore the
            transitive closure of their deps, runtime_deps and exports will be
            on the runtime classpath. By contrast, targets in the data
            attribute are included in the runfiles but on neither the
            compile-time nor runtime classpath.
        exports: Exported libraries.
        exported_plugins: The list of java_plugins (e.g. annotation processors)
            to export to libraries that directly depend on this library. The
            specified list of java_plugins will be applied to any library which
            directly depends on this library, just as if that library had
            explicitly declared these labels in plugins.
        neverlink: Whether this library should only be used for compilation and
            not at runtime. Useful if the library will be provided by the runtime
            environment during execution. Examples of such libraries are the IDE
            APIs for IDE plug-ins or tools.jar for anything running on a standard
            JDK.
        runtime_deps: Libraries to make available to the final binary or test
            at runtime only. Like ordinary deps, these will appear on the runtime
            classpath, but unlike them, not on the compile-time classpath.
            Dependencies needed only at runtime should be listed here.
            Dependency-analysis tools should ignore targets that appear in both
            runtime_deps and deps
        visibility: The visibility attribute on a target controls whether the
            target can be used in other packages. See the documentation for
            visibility.
        **kwargs: Additional key-word arguments that are passed to the internal
            java_library target.

    """

    # Build the private jar without the OSGI manifest
    private_library_name = "%s-no-manifest-do-not-use" % name
    kt_jvm_library(
        name = private_library_name,
        deps = deps,
        runtime_deps = runtime_deps,
        neverlink = True,
        visibility = ["//visibility:private"],
        **kwargs
    )

    # Repackage the jar with an OSGI manifest
    _osgi_kt_jvm_jar(
        name = name,
        automatic_module_name = automatic_module_name,
        bundle_description = bundle_description,
        bundle_doc_url = bundle_doc_url,
        bundle_license = bundle_license,
        bundle_name = bundle_name,
        bundle_symbolic_name = bundle_symbolic_name,
        bundle_version = bundle_version,
        export_package = bundle_additional_exports + ["*;version=${Bundle-Version}"],
        import_package = bundle_additional_imports + ["*"],
        target = private_library_name,
        deps = deps,
        runtime_deps = runtime_deps,
        exported_plugins = exported_plugins,
        neverlink = neverlink,
        exports = exports,
        visibility = visibility,
    )

def _run_osgi_wrapper(ctx, input_jar, output_jar):
    args = ctx.actions.args()
    args.add("--input_jar", input_jar.path)
    args.add("--output_jar", output_jar.path)
    args.add("--automatic_module_name", ctx.attr.automatic_module_name)
    args.add("--bundle_copyright", ctx.attr.bundle_copyright)
    args.add("--bundle_description", ctx.attr.bundle_description)
    args.add("--bundle_doc_url", ctx.attr.bundle_doc_url)
    args.add("--bundle_license", ctx.attr.bundle_license)
    args.add("--bundle_name", ctx.attr.bundle_name)
    args.add("--bundle_version", ctx.attr.bundle_version)
    args.add("--bundle_symbolic_name", ctx.attr.bundle_symbolic_name)
    args.add_joined("--export_package", ctx.attr.export_package, join_with = ",")
    args.add_joined("--import_package", ctx.attr.import_package, join_with = ",")

    ctx.actions.run(
        inputs = [input_jar],
        executable = ctx.executable._osgi_wrapper_exe,
        arguments = [args],
        outputs = [output_jar],
        progress_message = "Generating OSGi bundle Manifest for %s" % input_jar.path,
    )

# Kotlin implementation of osgi jar, removes classpath and source_jar
def _osgi_kt_jvm_jar_impl(ctx):
    if len(ctx.attr.target[JavaInfo].java_outputs) != 1:
        fail("osgi_jar rule can only be used on a single java target.")
    target_java_output = ctx.attr.target[JavaInfo].java_outputs[0]

    output_jar = ctx.outputs.output_jar

    input_jar = target_java_output.class_jar

    _run_osgi_wrapper(ctx, input_jar, output_jar)

    return [
        DefaultInfo(
            files = depset([output_jar]),
            # Workaround for https://github.com/bazelbuild/bazel/issues/15043
            # Bazel's native rule such as sh_test do not pick up 'files' in
            # DefaultInfo for a target in 'data'.
            data_runfiles = ctx.runfiles([output_jar]),
        ),
        JavaInfo(
            output_jar = output_jar,

            # compile_jar should be an ijar, but using an ijar results in
            # missing protobuf import version.
            compile_jar = output_jar,
            generated_class_jar = target_java_output.generated_class_jar,
            native_headers_jar = target_java_output.native_headers_jar,
            manifest_proto = target_java_output.manifest_proto,
            neverlink = ctx.attr.neverlink,
            deps = [dep[JavaInfo] for dep in ctx.attr.deps],
            runtime_deps = [dep[JavaInfo] for dep in ctx.attr.runtime_deps],
            exports = [exp[JavaInfo] for exp in ctx.attr.exports],
            exported_plugins = ctx.attr.exported_plugins,
        ),
    ]

_osgi_kt_jvm_jar = rule(
    implementation = _osgi_kt_jvm_jar_impl,
    outputs = {
        "output_jar": "lib%{name}.jar",
    },
    attrs = {
        "automatic_module_name": attr.string(),
        "bundle_copyright": attr.string(),
        "bundle_description": attr.string(),
        "bundle_doc_url": attr.string(),
        "bundle_license": attr.string(),
        "bundle_name": attr.string(),
        "bundle_version": attr.string(),
        "bundle_symbolic_name": attr.string(),
        "export_package": attr.string_list(),
        "import_package": attr.string_list(),
        "target": attr.label(),
        "deps": attr.label_list(),
        "runtime_deps": attr.label_list(),
        "exports": attr.label_list(),
        "neverlink": attr.bool(),
        "exported_plugins": attr.label_list(),
        "_osgi_wrapper_exe": attr.label(
            executable = True,
            cfg = "exec",
            allow_files = True,
            default = Label("//java/osgi:osgi_wrapper"),
        ),
    },
)
