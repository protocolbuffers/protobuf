"""Java options and protobuf-specific java build rules with those options."""
load("@rules_java//java:defs.bzl", "java_library")
load("@rules_jvm_external//:defs.bzl", "java_export")

JAVA_OPTS = [
    "-source 8",
    "-target 8",
    "-Xep:Java8ApiChecker:ERROR",
]

def protobuf_java_export(**kwargs):
    java_export(
        javacopts = JAVA_OPTS,
        **kwargs,
    )

def protobuf_java_library(**kwargs):
    java_library(
        javacopts = JAVA_OPTS,
        **kwargs,
    )
