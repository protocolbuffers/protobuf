""" Custom rule to generate OSGi Manifest """
load("@rules_java//java:defs.bzl", "java_library")

def osgi_java_library(name, bundle_description, bundle_doc_url, bundle_license, bundle_name, 
    bundle_symbolic_name, bundle_version, exports = [], visibility = [], neverlink = False, deps = [],
    runtime_deps = [], exported_plugins = [], **kwargs):
    # Build the private jar without the OSGI manifest
    private_library_name = "%s-no-manifest-do-not-use" % name
    java_library(
        name = private_library_name,
        deps = deps,
        runtime_deps = runtime_deps,
        neverlink = True,
        exported_plugins = exported_plugins,
        visibility = ["//visibility:private"],
        **kwargs
    )

    # Add the OSGI manifest
    _osgi_jar(
        name = name,
        bundle_description = bundle_description,
        bundle_doc_url = bundle_doc_url,
        bundle_license = bundle_license,
        bundle_name = bundle_name,
        bundle_symbolic_name = bundle_symbolic_name,
        bundle_version = bundle_version,
        export_package = ["*;version=${Bundle-Version}"],
        import_package = ["sun.misc;resolution:=optional", "*"],
        target = ":%s" % private_library_name,
        deps = deps,
        runtime_deps = runtime_deps,
        exported_plugins = exported_plugins,
        neverlink = neverlink,
        exports = exports,
        visibility = visibility,
    )

def _run_osgi_wrapper(ctx, input_jar, classpath_jars, output_jar):
    args = ctx.actions.args()
    args.add_joined("--classpath", classpath_jars, join_with = ":")
    args.add("--input_jar", input_jar.path)
    args.add("--output_jar", output_jar.path)
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
        inputs = [input_jar] + classpath_jars,
        executable = ctx.executable._osgi_wrapper_exe,
        arguments = [args],
        outputs = [output_jar],
        progress_message = "Generating OSGi bundle Manifest for %s" % input_jar.path,
    )

def _osgi_jar_impl(ctx):
    if len(ctx.attr.target[JavaInfo].java_outputs) != 1:
        fail("osgi_jar rule can only be used on a single java target.")

    target_java_output = ctx.attr.target[JavaInfo].java_outputs[0]

    if len(target_java_output.source_jars) > 1:
        fail("osgi_jar rule doesn't know how to deal with more than one source jar.")

    output_jar = ctx.outputs.output_jar

    input_jar = target_java_output.class_jar
    classpath_jars = ctx.attr.target[JavaInfo].compilation_info.compilation_classpath.to_list()

    _run_osgi_wrapper(ctx, input_jar, classpath_jars, output_jar)
    
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
            compile_jar = output_jar, #ijar, #using ijar results in missing protobuf import version.
            source_jar = target_java_output.source_jars[0],
            compile_jdeps = target_java_output.compile_jdeps,
            generated_class_jar = target_java_output.generated_class_jar,
            generated_source_jar = target_java_output.generated_source_jar,
            native_headers_jar = target_java_output.native_headers_jar,
            manifest_proto = target_java_output.manifest_proto,
            neverlink = ctx.attr.neverlink,
            deps = [dep[JavaInfo] for dep in ctx.attr.deps],
            runtime_deps = [dep[JavaInfo] for dep in ctx.attr.runtime_deps],
            exports = [exp[JavaInfo] for exp in ctx.attr.exports],
            exported_plugins = ctx.attr.exported_plugins,
            jdeps = target_java_output.jdeps,
        ),
    ]

_osgi_jar = rule(
    attrs = {
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
        "_java_toolchain": attr.label(
            default = "@bazel_tools//tools/jdk:current_java_toolchain",
        ),
    },
    fragments = ["java"],
    outputs = {
        "output_jar": "lib%{name}.jar",
    },
    implementation = _osgi_jar_impl,
)
