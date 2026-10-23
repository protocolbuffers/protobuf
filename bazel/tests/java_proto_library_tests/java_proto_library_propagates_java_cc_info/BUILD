load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_java//java:java_library.bzl", "java_library")
load("@rules_testing//lib:util.bzl", "PREVENT_IMPLICIT_BUILDING_TAGS")
load("//bazel:java_proto_library.bzl", "java_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//bazel/tests:proto_bzl_test_suite.bzl", "bzl_test_suite", "package_label_string")
load("//bazel/toolchains:proto_lang_toolchain.bzl", "proto_lang_toolchain")
load(":tests.bzl", "TESTS", "TEST_ATTRS")

package(default_applicable_licenses = ["//:license"])

proto_library(
    name = "bar_proto",
    srcs = ["bar.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_library(
    name = "foo_proto",
    srcs = ["foo.proto"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":bar_proto"],
)

java_proto_library(
    name = "foo_java_proto",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":foo_proto"],
)

# The runtime of the Java proto toolchain below, with a native dependency that
# java_proto_library must propagate.
java_library(
    name = "java_runtime",
    srcs = ["java_runtime.java"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
    deps = [":native_dep_of_runtime"],
)

cc_library(
    name = "native_dep_of_runtime",
    srcs = ["native_dep_of_runtime.cc"],
    hdrs = ["native_dep_of_runtime.h"],
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

proto_lang_toolchain(
    name = "java",
    command_line = "--java_out=shared,immutable:$(OUT)",
    progress_message = "Generating Java (Immutable) proto_library %{label}",
    runtime = ":java_runtime",
    tags = PREVENT_IMPLICIT_BUILDING_TAGS,
)

bzl_test_suite(
    name = "tests",
    attrs = TEST_ATTRS,
    config_settings = {
        "//bazel/flags/java:proto_toolchain_for_java": package_label_string(label_str = ":java"),
    },
    tests = TESTS,
)
