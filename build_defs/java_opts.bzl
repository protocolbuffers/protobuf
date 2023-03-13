"""Java options and protobuf-specific java build rules with those options."""

load("@rules_java//java:defs.bzl", "java_library")
load("@rules_jvm_external//:defs.bzl", "java_export")
load("//java/osgi:osgi.bzl", "osgi_jar")

JAVA_OPTS = [
    "-source 8",
    "-target 8",
    "-Xep:Java8ApiChecker:ERROR",
]

BUNDLE_DOC_URL = "https://developers.google.com/protocol-buffers/"
BUNDLE_LICENSE = "https://opensource.org/licenses/BSD-3-Clause"

def protobuf_java_export(**kwargs):
    java_export(
        javacopts = JAVA_OPTS,
        **kwargs
    )

def protobuf_java_library(**kwargs):
    java_library(
        javacopts = JAVA_OPTS,
        **kwargs
    )

def protobuf_versioned_java_library(name, bundle_description, bundle_name, bundle_symbolic_name, bundle_version, exports = [], visibility = [], **kwargs):
    # Build the private jar without the OSGI manifest
    protobuf_java_library(
        name = name + "-no-manifest",
        visibility = ["//visibility:private"],
        **kwargs
    )

    # Add the OSGI manifest
    osgi_jar(
        name = name,
        bundle_description = bundle_description,
        bundle_doc_url = BUNDLE_DOC_URL,
        bundle_license = BUNDLE_LICENSE,
        bundle_name = bundle_name,
        bundle_symbolic_name = bundle_symbolic_name,
        bundle_version = bundle_version,
        export_package = ["*;version=${Bundle-Version}"],
        import_package = ["sun.misc;resolution:=optional", "*"],
        target = ":" + name + "-no-manifest",
        exports = exports,
        visibility = visibility,
    )
