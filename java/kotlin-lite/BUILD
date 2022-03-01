load("@io_bazel_rules_kotlin//kotlin:jvm.bzl", "kt_jvm_library")
load("@rules_java//java:defs.bzl", "java_lite_proto_library")
load("@rules_jvm_external//:kt_defs.bzl", "kt_jvm_export")
load("//:protobuf_version.bzl", "PROTOBUF_VERSION")
load("//:protobuf.bzl", "internal_gen_kt_protos")

java_lite_proto_library(
    name = "example_extensible_message_java_proto_lite",
    deps = ["//java/kotlin:example_extensible_message_proto"],
)

kt_jvm_library(
    name = "lite_extensions",
    srcs = ["src/main/kotlin/com/google/protobuf/ExtendableMessageLiteExtensions.kt"],
    deps = ["//java/lite"],
)

kt_jvm_library(
    name = "well_known_protos_kotlin_lite",
    srcs = [
        "//:gen_well_known_protos_kotlinlite",
    ],
    deps = [
        "//java/lite",
        "//java/kotlin:only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/kotlin:shared_runtime",
    ],
)

kt_jvm_export(
    name = "kotlin-lite_mvn",
    maven_coordinates = "com.google.protobuf:protobuf-kotlin-lite:%s" % PROTOBUF_VERSION,
    pom_template = "//java/kotlin-lite:pom_template.xml",
    resources = ["//:well_known_protos"],
    runtime_deps = [
        ":lite_extensions",
        ":well_known_protos_kotlin_lite",
        "//java/kotlin:bytestring_lib",
        "//java/kotlin:only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/kotlin:shared_runtime",
    ],
    deploy_env = [
        "@com_github_jetbrains_kotlin//:kotlin-stdlib",
        "//java/lite",
    ],
)

filegroup(
    name = "release",
    srcs = [
        ":kotlin-lite_mvn-docs",
        ":kotlin-lite_mvn-maven-source",
        ":kotlin-lite_mvn-pom",
        ":kotlin-lite_mvn-project",
    ],
    visibility = ["//java:__pkg__"],
)

test_suite(
    name = "tests",
    tests = [
        "test_lite_extensions",
        "proto2_test_lite",
        "proto3_test_lite",
    ],
)

kt_jvm_library(
    name = "test_lite_extensions_library",
    srcs = ["src/test/kotlin/com/google/protobuf/ExtendableMessageLiteExtensionsTest.kt"],
    deps = [
        ":example_extensible_message_java_proto_lite",
        ":lite_extensions",
        "//java/lite",
        "//java/kotlin:only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/kotlin:shared_runtime",
        "@com_github_jetbrains_kotlin//:kotlin-test",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "test_lite_extensions",
    runtime_deps = [":test_lite_extensions_library"],
    test_class = "com.google.protobuf.kotlin.ExtendableMessageLiteExtensionsTest",
)

java_lite_proto_library(
    name = "evil_names_proto2_java_proto_lite",
    deps = ["//java/kotlin:evil_names_proto2"],
)

internal_gen_kt_protos(
    name = "gen_evil_names_proto2_lite",
    deps = ["//java/kotlin:evil_names_proto2"],
    lite = True,
)

java_lite_proto_library(
    name = "evil_names_proto3_java_proto_lite",
    deps = ["//java/kotlin:evil_names_proto3"],
)

internal_gen_kt_protos(
    name = "gen_evil_names_proto3_lite",
    deps = ["//java/kotlin:evil_names_proto3"],
    lite = True,
)

java_lite_proto_library(
    name = "multiple_files_proto3_java_proto_lite",
    deps = ["//java/kotlin:multiple_files_proto3"],
)

internal_gen_kt_protos(
    name = "gen_kotlin_proto3_java_multiple_files_lite",
    deps = ["//java/kotlin:multiple_files_proto3"],
)

kt_jvm_library(
    name = "kotlin_unittest_lite",
    srcs = [
        ":gen_evil_names_proto2_lite",
        "//:gen_kotlin_unittest_lite",
    ],
    deps = [
        ":evil_names_proto2_java_proto_lite",
        "//java/lite:lite",
        "//java/kotlin:only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/kotlin:shared_runtime",
        "//:java_lite_test_protos",
    ],
)

kt_jvm_library(
    name = "kotlin_proto3_unittest_lite",
    srcs = [
        ":gen_evil_names_proto3_lite",
        ":gen_kotlin_proto3_java_multiple_files_lite",
        "//:gen_kotlin_proto3_unittest_lite",
    ],
    deps = [
        ":evil_names_proto3_java_proto_lite",
        ":multiple_files_proto3_java_proto_lite",
        "//java/lite:lite",
        "//java/kotlin:only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/kotlin:shared_runtime",
        "//:java_lite_test_protos",
    ],
)

kt_jvm_library(
    name = "proto2_test_lite_library",
    srcs = ["src/test/kotlin/com/google/protobuf/Proto2LiteTest.kt"],
    deps = [
        ":kotlin_unittest_lite",
        "//java/core:test_util_lite",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "proto2_test_lite",
    runtime_deps = [":proto2_test_lite_library"],
    test_class = "com.google.protobuf.kotlin.Proto2LiteTest",
)

kt_jvm_library(
    name = "proto3_test_lite_library",
    srcs = ["//java/kotlin:src/test/kotlin/com/google/protobuf/Proto3Test.kt"],
    deps = [
        ":kotlin_proto3_unittest_lite",
        "//java/core:test_util_lite",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "proto3_test_lite",
    runtime_deps = [":proto3_test_lite_library"],
    test_class = "com.google.protobuf.kotlin.Proto3Test",
)
