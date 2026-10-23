load("@rules_java//java:java_library.bzl", "java_library")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite", "package_label_string")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load(":tests.bzl", "TESTS")

package(default_applicable_licenses = ["//:license"])

# The proto that is "provided" by the runtime (i.e. denylisted by the toolchain).
proto_library(
    name = "provided_proto",
    srcs = ["provided.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

# Mirrors the runtime's aggregate proto_library that the toolchain denylists;
# :provided_proto is denylisted transitively through it.
proto_library(
    name = "protobuf_proto_sources",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":provided_proto"],
)

# Mixes a provided (denylisted) proto file with a non-provided one, which is
# an error for java_proto_library.
proto_library(
    name = "mixed_proto",
    srcs = [
        "non_provided.proto",
        "provided.proto",
    ],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

java_proto_library(
    name = "mixed_java_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":mixed_proto"],
)

java_library(
    name = "java_runtime",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_lang_toolchain(
    name = "java",
    command_line = "--java_out=shared,immutable:$(OUT)",
    denylisted_protos = [":protobuf_proto_sources"],
    progress_message = "Generating Java (Immutable) proto_library %{label}",
    runtime = ":java_runtime",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

bzl_test_suite(
    name = "tests",
    config_settings = {
        "//bazel/flags/java:proto_toolchain_for_java": package_label_string(label_str = ":java"),
    },
    expect_failure = True,
    tests = TESTS,
)
