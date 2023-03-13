""" Custom rule to generate OSGi Manifest """

def _osgi_jar_impl(ctx):
    output = ctx.outputs.osgi_jar.path
    input_jar = ctx.attr.target[JavaInfo].outputs.jars[0].class_jar
    classpath_jars = ctx.attr.target[JavaInfo].compilation_info.compilation_classpath

    args = ctx.actions.args()
    args.add_joined("--classpath", classpath_jars, join_with = ":")
    args.add("--input_jar", input_jar.path)
    args.add("--output_jar", output)
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
        inputs = [input_jar] + classpath_jars.to_list(),
        executable = ctx.executable._osgi_wrapper_exe,
        arguments = [args],
        outputs = [ctx.outputs.osgi_jar],
        progress_message = "Generating OSGi bundle Manifest for %s" % ctx.attr.target.label,
    )

    return [
        DefaultInfo(),
        JavaInfo(
            output_jar = ctx.outputs.osgi_jar,
            compile_jar = ctx.outputs.osgi_jar,
            source_jar = ctx.attr.target[JavaInfo].source_jars[0],
            #compile_jdeps = ctx.attr.target[JavaInfo].outputs.jdeps,
            #generated_class_jar = ctx.attr.target[JavaInfo].generated_class_jar,
            #generated_source_jar = ctx.attr.target[JavaInfo].generated_source_jar,
            native_headers_jar = ctx.attr.target[JavaInfo].outputs.native_headers,
            #manifest_proto = ctx.attr.target[JavaInfo].manifest_proto,
            #neverlink = ctx.attr.target[JavaInfo].neverlink,
            #deps = ctx.attr.target[JavaInfo].deps,
            #runtime_deps = ctx.attr.target[JavaInfo].runtime_deps,
            exports = [exp[JavaInfo] for exp in ctx.attr.exports],
            #exported_plugins = ctx.attr.target[JavaInfo].exported_plugins,
            jdeps = ctx.attr.target[JavaInfo].outputs.jdeps,
            #native_libraries = ctx.attr.target[JavaInfo].native_libraries,
        ),
    ]

osgi_jar = rule(
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
        "exports": attr.label_list(),
        "_osgi_wrapper_exe": attr.label(
            executable = True,
            cfg = "exec",
            allow_files = True,
            default = Label("//java/osgi:osgi_wrapper"),
        ),
    },
    fragments = ["java"],
    outputs = {
        "osgi_jar": "lib%{name}.jar",
    },
    implementation = _osgi_jar_impl,
)
