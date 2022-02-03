load("@io_bazel_rules_kotlin//kotlin:jvm.bzl", "kt_jvm_library")
load("@rules_java//java:defs.bzl", "java_lite_proto_library")

java_lite_proto_library(
    name = "example_extensible_message_java_proto_lite",
    deps = ["//java/kotlin:example_extensible_message_proto"],
)

kt_jvm_library(
    name = "lite_extensions",
    srcs = ["src/main/kotlin/com/google/protobuf/ExtendableMessageLiteExtensions.kt"],
    deps = ["//java/lite"],
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

java_lite_proto_library(
    name = "evil_names_proto3_java_proto_lite",
    deps = ["//java/kotlin:evil_names_proto3"],
)

java_lite_proto_library(
    name = "multiple_files_proto3_java_proto_lite",
    deps = ["//java/kotlin:multiple_files_proto3"],
)

genrule(
    name = "gen_kotlin_proto3_java_multiple_files_lite",
    srcs = ["src/test/proto/com/google/protobuf/multiple_files_proto3.proto"],
    outs = [
        "MultipleFilesMessageALiteKt.kt",
        "MultipleFilesMessageBLiteKt.kt",
        "MultipleFilesProto3LiteKt.kt",
    ],
    cmd = "$(location //:protoc) " +
          "--kotlin_out=lite:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/multiple_files_proto3.proto) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/MultipleFilesMessageAKt.kt " +
          "$(location MultipleFilesMessageALiteKt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/MultipleFilesMessageBKt.kt " +
          "$(location MultipleFilesMessageBLiteKt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/MultipleFilesProto3Kt.kt " +
          "$(location MultipleFilesProto3LiteKt.kt)",
    tools = ["//:protoc"],
)

genrule(
    name = "gen_evil_names_proto2_lite",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto2.proto"],
    outs = [
        "EvilNamesProto2LiteKt.kt",
        "HardKeywordsAllTypesProto2LiteKt.kt",
        "InterfaceKt.kt",
    ],
    cmd = "$(location //:protoc) " +
          "--kotlin_out=lite:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/evil_names_proto2.proto) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/EvilNamesProto2Kt.kt " +
          "$(location EvilNamesProto2LiteKt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/HardKeywordsAllTypesProto2Kt.kt " +
          "$(location HardKeywordsAllTypesProto2LiteKt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/InterfaceKt.kt " +
          "$(location InterfaceKt.kt)",
    tools = ["//:protoc"],
)

genrule(
    name = "gen_evil_names_proto3_lite",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto3.proto"],
    outs = [
        "ClassKt.kt",
        "EvilNamesProto3Kt.kt",
        "HardKeywordsAllTypesProto3Kt.kt",
    ],
    cmd = "$(location //:protoc) " +
          "--kotlin_out=lite:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/evil_names_proto3.proto) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/ClassKt.kt " +
          "$(location ClassKt.kt) && " + 
          "cp $(@D)/com/google/protobuf/kotlin/generator/EvilNamesProto3Kt.kt " +
          "$(location EvilNamesProto3Kt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/HardKeywordsAllTypesProto3Kt.kt " +
          "$(location HardKeywordsAllTypesProto3Kt.kt)",
    tools = ["//:protoc"],
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
    srcs = ["src/test/kotlin/com/google/protobuf/Proto3LiteTest.kt"],
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
    test_class = "com.google.protobuf.kotlin.Proto3LiteTest",
)
