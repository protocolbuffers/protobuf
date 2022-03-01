load("@io_bazel_rules_kotlin//kotlin:jvm.bzl", "kt_jvm_library")
load("@rules_java//java:defs.bzl", "java_proto_library")
load("@rules_jvm_external//:kt_defs.bzl", "kt_jvm_export")
load("@rules_proto//proto:defs.bzl", "proto_library")
load("//:protobuf_version.bzl", "PROTOBUF_VERSION")
load("//:protobuf.bzl", "internal_gen_kt_protos")

exports_files([
  "src/test/kotlin/com/google/protobuf/Proto3Test.kt",
])

# Kotlin generated protos depend on this and only this.
kt_jvm_library(
    name = "shared_runtime",
    srcs = [
        "src/main/kotlin/com/google/protobuf/DslList.kt",
        "src/main/kotlin/com/google/protobuf/DslMap.kt",
        "src/main/kotlin/com/google/protobuf/DslProxy.kt",
        "src/main/kotlin/com/google/protobuf/ExtensionList.kt",
        "src/main/kotlin/com/google/protobuf/ProtoDslMarker.kt",
        "src/main/kotlin/com/google/protobuf/UnmodifiableCollections.kt",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/lite",
    ],
)

kt_jvm_library(
    name = "only_for_use_in_proto_generated_code_its_generator_and_tests",
    srcs = ["src/main/kotlin/com/google/protobuf/OnlyForUseByGeneratedProtoCode.kt"],
    visibility = ["//java:__subpackages__"],
)

kt_jvm_library(
    name = "bytestring_lib",
    srcs = ["src/main/kotlin/com/google/protobuf/ByteStrings.kt"],
    deps = ["//java/lite"],
    visibility = ["//java:__subpackages__"],
)

kt_jvm_library(
    name = "full_extensions",
    srcs = [
      "src/main/kotlin/com/google/protobuf/Anies.kt",
      "src/main/kotlin/com/google/protobuf/ExtendableMessageExtensions.kt",
    ],
    deps = ["//java/core"],
)

kt_jvm_export(
    name = "kotlin_mvn",
    maven_coordinates = "com.google.protobuf:protobuf-kotlin:%s" % PROTOBUF_VERSION,
    pom_template = "//java/kotlin:pom_template.xml",
    resources = ["//:well_known_protos"],
    runtime_deps = [
      ":bytestring_lib",
      ":full_extensions",
      ":only_for_use_in_proto_generated_code_its_generator_and_tests",
      ":shared_runtime",
      ":well_known_protos_kotlin",
    ],
    deploy_env = [
        "@com_github_jetbrains_kotlin//:kotlin-stdlib",
        "//java/core",
    ],
)

filegroup(
    name = "release",
    srcs = [
        ":kotlin_mvn-docs",
        ":kotlin_mvn-maven-source",
        ":kotlin_mvn-pom",
        ":kotlin_mvn-project",
    ],
    visibility = ["//java:__pkg__"],
)

test_suite(
    name = "tests",
    tests = [
        "bytestring_test",
        "shared_tests",
        "test_extensions",
        "proto2_test",
        "proto3_test",
    ],
)

kt_jvm_library(
    name = "bytestring_test_library",
    srcs = ["src/test/kotlin/com/google/protobuf/ByteStringsTest.kt"],
    deps = [
        ":bytestring_lib",
        "//java/lite",
        "@com_github_jetbrains_kotlin//:kotlin-test",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "bytestring_test",
    runtime_deps = [":bytestring_test_library"],
    test_class = "com.google.protobuf.kotlin.ByteStringsTest",
)

proto_library(
    name = "example_extensible_message_proto",
    srcs = ["src/test/proto/com/google/protobuf/example_extensible_message.proto"],
    visibility = ["//java:__subpackages__"],
)

java_proto_library(
    name = "example_extensible_message_java_proto",
    deps = [":example_extensible_message_proto"],
)

kt_jvm_library(
    name = "shared_tests_library",
    srcs = [
        "src/test/kotlin/com/google/protobuf/DslListTest.kt",
        "src/test/kotlin/com/google/protobuf/DslMapTest.kt",
        "src/test/kotlin/com/google/protobuf/ExtensionListTest.kt",
    ],
    deps = [
        ":bytestring_lib",
        ":example_extensible_message_java_proto",
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        ":shared_runtime",
        "@com_github_jetbrains_kotlin//:kotlin-test",
        "@maven//:com_google_truth_truth",
        "@maven//:com_google_guava_guava_testlib",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "shared_tests",
    runtime_deps = [":shared_tests_library"],
    test_class = "com.google.protobuf.kotlin.DslListTest",
)

kt_jvm_library(
    name = "test_extensions_library",
    srcs = [
      "src/test/kotlin/com/google/protobuf/AniesTest.kt",
      "src/test/kotlin/com/google/protobuf/ExtendableMessageExtensionsTest.kt",
    ],
    deps = [
        ":example_extensible_message_java_proto",
        ":full_extensions",
        "//java/core:core",
        ":kotlin_unittest",
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        ":shared_runtime",
        "@com_github_jetbrains_kotlin//:kotlin-test",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "test_extensions",
    runtime_deps = [":test_extensions_library"],
    test_class = "com.google.protobuf.kotlin.ExtendableMessageExtensionsTest",
)

proto_library(
    name = "evil_names_proto2",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto2.proto"],
    visibility = ["//:__subpackages__"],
)

java_proto_library(
    name = "evil_names_proto2_java_proto",
    deps = [":evil_names_proto2"],
)

internal_gen_kt_protos(
    name = "gen_evil_names_proto2",
    deps = [":evil_names_proto2"],
)

proto_library(
    name = "evil_names_proto3",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto3.proto"],
    visibility = ["//:__subpackages__"],
)

java_proto_library(
    name = "evil_names_proto3_java_proto",
    deps = [":evil_names_proto3"],
)

internal_gen_kt_protos(
    name = "gen_evil_names_proto3",
    deps = [":evil_names_proto3"],
)

proto_library(
    name = "multiple_files_proto3",
    srcs = ["src/test/proto/com/google/protobuf/multiple_files_proto3.proto"],
    visibility = ["//:__subpackages__"],
)

java_proto_library(
    name = "multiple_files_proto3_java_proto",
    deps = [":multiple_files_proto3"],
)

internal_gen_kt_protos(
    name = "gen_kotlin_proto3_java_multiple_files",
    deps = [":multiple_files_proto3"],
)

kt_jvm_library(
    name = "kotlin_unittest",
    srcs = [
        ":gen_evil_names_proto2",
        "//:gen_kotlin_unittest",
    ],
    deps = [
        ":evil_names_proto2_java_proto",
        "//java/core:core",
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        ":shared_runtime",
        ":well_known_protos_kotlin",
        "//:java_test_protos",
    ],
)

kt_jvm_library(
    name = "kotlin_proto3_unittest",
    srcs = [
        ":gen_evil_names_proto3",
        ":gen_kotlin_proto3_java_multiple_files",
        "//:gen_kotlin_proto3_unittest",
    ],
    deps = [
        ":evil_names_proto3_java_proto",
        ":multiple_files_proto3_java_proto",
        "//java/core:core",
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        ":shared_runtime",
        "//:java_test_protos",
    ],
)

kt_jvm_library(
    name = "proto2_test_library",
    srcs = ["src/test/kotlin/com/google/protobuf/Proto2Test.kt"],
    deps = [
        ":kotlin_unittest",
        "//java/core:test_util",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "proto2_test",
    runtime_deps = [":proto2_test_library"],
    test_class = "com.google.protobuf.kotlin.Proto2Test",
)

kt_jvm_library(
    name = "proto3_test_library",
    srcs = ["src/test/kotlin/com/google/protobuf/Proto3Test.kt"],
    deps = [
        ":kotlin_proto3_unittest",
        "//java/core:test_util",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

java_test(
    name = "proto3_test",
    runtime_deps = [":proto3_test_library"],
    test_class = "com.google.protobuf.kotlin.Proto3Test",
)

kt_jvm_library(
    name = "well_known_protos_kotlin",
    srcs = [
        "//:gen_well_known_protos_kotlin",
    ],
    deps = [
         "//java/core",
         ":only_for_use_in_proto_generated_code_its_generator_and_tests",
         ":shared_runtime",
    ],
)
