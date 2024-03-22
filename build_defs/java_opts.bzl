"""Java options and protobuf-specific java build rules with those options."""

load("@rules_java//java:defs.bzl", "java_library", "java_import")
load("@rules_jvm_external//:defs.bzl", "java_export")
load("//java/osgi:osgi.bzl", "osgi_java_library")
load("//:protobuf_version.bzl", "PROTOBUF_JAVA_VERSION")

BUNDLE_DOC_URL = "https://developers.google.com/protocol-buffers/"
BUNDLE_LICENSE = "https://opensource.org/licenses/BSD-3-Clause"

## JDK target to use for versioned classes.
JPMS_JDK_TARGET = "9"

## Java compiler options for JDK 8.
JAVA_OPTS = [
    "-source 8",
    "-target 8",
    "-Xep:Java8ApiChecker:ERROR",
]

## Java compiler options for JDK 9+.
JAVA9_OPTS = [
    "-source 9",
    "-target 9",
]

## Default JAR manifest entries to stamp on export.
DEFAULT_MANIFEST_ENTRIES = {
    "Multi-Release": "true"
}

## Default visibility settings for Java module targets.
DEFAULT_JMOD_VISIBILITY = [
    "//java:__subpackages__",
]

def protobuf_java_export(**kwargs):
    java_export(
        javacopts = JAVA_OPTS,
        manifest_entries = DEFAULT_MANIFEST_ENTRIES,
        **kwargs
    )

def protobuf_java_library(**kwargs):
    java_library(
        javacopts = JAVA_OPTS,
        **kwargs
    )

def protobuf_java_module(
    name,
    module,
    module_deps = [],
    deps = [],
    jvm_version = JPMS_JDK_TARGET,
    visibility = DEFAULT_JMOD_VISIBILITY,
    **kwargs):
    """Builds a `module-info.java` definition for use with a Protobuf Java target.

    This macro replaces a chain of `java_library` and `genrule` steps to coax Bazel
    into placing the `module-info.class` at the right path in a versioned modular
    MRJAR (Multi-Release JAR).

    The name provided to this target is then meant to be used downstream as a
    JAR-unwrapped classfile. For example:

    ```starlark
    protobuf_java_module(
        name = "my-module",
        module = "src/some/module-info.java",
    )
    java_library(
        name = "...",
        resources = [":my-module"],
        resource_strip_prefix = "src/some/",
    )
    ```

    Args:
        name: (required) The target name; a naked `.class` file
        module: (required) Source file path for the `module-info.java`
        module_deps: Other `protobuf_java_module` targets this one depends on
        deps: Regular dependencies to add to the `java_library` target
        jvm_version: JVM version for the path in the MRJAR; defaults to `9`.
        visibility: Visibility setting to give the created targets.
        **kwargs: Keyword arguments to pass to `java_library`
    """

    # like: `module-java9`
    module_target_name = "module-java%s" % jvm_version

    # like: `module-java9-class`
    module_class_target_name = "module-java%s-class" % jvm_version

    # like: `module-java9-[zip|jar]`
    zip_target_name = "module-java%s-zip" % jvm_version
    jar_target_name = "%s-jar" % name
    module_compilejar_target_name = "%s-compilejar" % name

    # like: `META-INF/versions/9`
    class_prefix = "META-INF/versions/%s" % jvm_version
    class_file = "%s/module-info.class" % class_prefix
    repacked_jar_file = "module-java%s.jar" % jvm_version

    # command:
    # mkdir -p `META-INF/versions/9` &&
    # unzip -p module.jar module-info.class > `META-INF/verisons/9/module-info.class`
    extract_classfile_command = " && ".join([
        "mkdir -p %s" % class_prefix,
        "unzip -p $(location :module-java%s) module-info.class > $@" % jvm_version,
    ])

    # command:
    # zip -r - META-INF > module-java9.jar
    repack_zip_command = " && ".join([
        "mkdir -p %s" % class_prefix,
        "unzip -p $(location :module-java%s) module-info.class > %s" % (jvm_version, class_file),
        "zip -r - META-INF > $@",
    ])

    deps_rewritten = []
    if len(module_deps) > 0:
        deps_rewritten = [
            "%s-compilejar" % i for i in module_deps
        ]

    java_library(
        name = module_target_name,
        srcs = [module],
        javacopts = JAVA9_OPTS,
        deps = deps_rewritten + (deps or []),
        visibility = visibility,
        **kwargs,
    )
    native.genrule(
        name = module_class_target_name,
        srcs = [module_target_name],
        outs = [class_file],
        cmd = extract_classfile_command,
        visibility = visibility,
    )
    native.genrule(
        name = zip_target_name,
        srcs = [module_target_name],
        outs = [repacked_jar_file],
        cmd = repack_zip_command,
    )
    java_import(
        name = jar_target_name,
        jars = [zip_target_name],
        visibility = visibility,
    )
    native.alias(
        name = name,
        actual = module_class_target_name,
        visibility = visibility,
    )
    native.alias(
        name = module_compilejar_target_name,
        actual = module_target_name,
        visibility = visibility,
    )

def protobuf_versioned_java_library(
        module_name,
        bundle_description,
        bundle_name,
        bundle_symbolic_name,
        bundle_additional_imports = [],
        bundle_additional_exports = [],
        **kwargs):
    """Extends `java_library` to add OSGi headers to the MANIFEST.MF using bndlib

    This macro should be usable as a drop-in replacement for java_library.

    The additional arguments are given the bndlib tool to generate an OSGi-compliant manifest file.
    See [bnd documentation](https://bnd.bndtools.org/chapters/110-introduction.html)

    Takes all the args that are standard for a java_library target plus the following.
    Args:
        bundle_description: (required) The Bundle-Description header defines a short
            description of this bundle.
        module_name: (required) The Java 9 module name for this target.
        bundle_name: (required) The Bundle-Name header defines a readable name for this
            bundle. This should be a short, human-readable name that can
            contain spaces.
        bundle_symbolic_name: (required) The Bundle-SymbolicName header specifies a
            non-localizable name for this bundle. The bundle symbolic name
            together with a version must identify a unique bundle though it can
            be installed multiple times in a framework. The bundle symbolic
            name should be based on the reverse domain name convention.
        bundle_additional_exports: The Export-Package header contains a
            declaration of exported packages. These are additional export
            package statements to be added before the default wildcard export
            "*;version={$Bundle-Version}".
        bundle_additional_imports: The Import-Package header declares the
            imported packages for this bundle. These are additional import
            package statements to be added before the default wildcard import
            "*".
        **kwargs: Additional key-word arguments that are passed to the internal
            java_library target.
    """
    osgi_java_library(
        javacopts = JAVA_OPTS,
        module_name = module_name,
        bundle_doc_url = BUNDLE_DOC_URL,
        bundle_license = BUNDLE_LICENSE,
        bundle_version = PROTOBUF_JAVA_VERSION,
        bundle_description = bundle_description,
        bundle_name = bundle_name,
        bundle_symbolic_name = bundle_symbolic_name,
        bundle_additional_exports = bundle_additional_exports,
        bundle_additional_imports = bundle_additional_imports + ["sun.misc;resolution:=optional"],
        **kwargs
    )
