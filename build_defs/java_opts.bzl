"""Java options and protobuf-specific java build rules with those options."""

load("@rules_java//java:defs.bzl", "java_library")
load("@rules_jvm_external//:defs.bzl", "java_export")
load("//java/osgi:osgi.bzl", "osgi_java_library")
load("//:protobuf_version.bzl", "PROTOBUF_JAVA_VERSION")

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

def protobuf_versioned_java_library(**kwargs):
    osgi_java_library(
        javacopts = JAVA_OPTS,
        bundle_doc_url = BUNDLE_DOC_URL,
        bundle_license = BUNDLE_LICENSE,
        bundle_version = PROTOBUF_JAVA_VERSION,
        bundle_additional_imports = ["sun.misc;resolution:=optional"],
        **kwargs
    )
