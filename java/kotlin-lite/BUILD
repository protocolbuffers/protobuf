load("@io_bazel_rules_kotlin//kotlin:jvm.bzl", "kt_jvm_library")
load("//java/internal:testing.bzl", "junit_tests")
load("@rules_java//java:defs.bzl", "java_lite_proto_library", "java_proto_library")
load("@rules_proto//proto:defs.bzl", "proto_library")


# Kotlin generated protos depend on this and only this.
kt_jvm_library(
    name = "shared_runtime",
    srcs = [
        "src/main/kotlin/com/google/protobuf/DslList.kt",
        "src/main/kotlin/com/google/protobuf/DslMap.kt",
        "src/main/kotlin/com/google/protobuf/DslProxy.kt",
        "src/main/kotlin/com/google/protobuf/ExtensionList.kt",
        "src/main/kotlin/com/google/protobuf/ProtoDslMarker.kt",
        "UnmodifiableCollections.kt",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/lite:lite",
    ],
)

kt_jvm_library(
    name = "only_for_use_in_proto_generated_code_its_generator_and_tests",
    srcs = ["OnlyForUseByGeneratedProtoCode.kt"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "evil_names_proto2",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto2.proto"],
)

proto_library(
    name = "evil_names_proto3",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto3.proto"],
)

java_lite_proto_library(
    name = "evil_names_proto2_java_proto_lite",
    deps = [":evil_names_proto2"],
)

java_proto_library(
    name = "evil_names_proto2_java_proto",
    deps = [":evil_names_proto2"],
)

java_lite_proto_library(
    name = "evil_names_proto3_java_proto_lite",
    deps = [":evil_names_proto3"],
)

java_proto_library(
    name = "evil_names_proto3_java_proto",
    deps = [":evil_names_proto3"],
)

proto_library(
    name = "multiple_files_proto3",
    srcs = ["src/test/proto/com/google/protobuf/multiple_files_proto3.proto"],
)

java_lite_proto_library(
    name = "multiple_files_proto3_java_proto_lite",
    deps = [":multiple_files_proto3"],
)

java_proto_library(
    name = "multiple_files_proto3_java_proto",
    deps = [":multiple_files_proto3"],
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
          "--kotlin_out=immutable:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/multiple_files_proto3.proto) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/MultipleFilesMessageAKt.kt " +
          "$(location MultipleFilesMessageALiteKt.kt) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/MultipleFilesMessageBKt.kt " +
          "$(location MultipleFilesMessageBLiteKt.kt) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/MultipleFilesProto3Kt.kt " +
          "$(location MultipleFilesProto3LiteKt.kt)",
    tools = ["//:protoc"],
)

genrule(
    name = "gen_kotlin_proto3_java_multiple_files",
    srcs = ["src/test/proto/com/google/protobuf/multiple_files_proto3.proto"],
    outs = [
        "MultipleFilesMessageAKt.kt",
        "MultipleFilesMessageBKt.kt",
        "MultipleFilesProto3Kt.kt",
    ],
    cmd = "$(location //:protoc) " +
          "--kotlin_out=immutable:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/multiple_files_proto3.proto) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/MultipleFilesMessageAKt.kt " +
          "$(location MultipleFilesMessageAKt.kt) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/MultipleFilesMessageBKt.kt " +
          "$(location MultipleFilesMessageBKt.kt) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/MultipleFilesProto3Kt.kt " +
          "$(location MultipleFilesProto3Kt.kt)",
    tools = ["//:protoc"],
)

genrule(
    name = "gen_evil_names_proto2_lite",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto2.proto"],
    outs = [
        "EvilNamesProto2LiteKt.kt",
        "HardKeywordsAllTypesProto2LiteKt.kt",
    ],
    cmd = "$(location //:protoc) " +
          "--kotlin_out=lite:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/evil_names_proto2.proto) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/EvilNamesProto2Kt.kt " +
          "$(location EvilNamesProto2LiteKt.kt) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/HardKeywordsAllTypesKt.kt " +
          "$(location HardKeywordsAllTypesProto2LiteKt.kt)",
    tools = ["//:protoc"],
)

genrule(
    name = "gen_evil_names_proto2",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto2.proto"],
    outs = [
        "EvilNamesProto2Kt.kt",
        "HardKeywordsAllTypesProto2Kt.kt",
    ],
    cmd = "$(location //:protoc) " +
          "--kotlin_out=immutable:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/evil_names_proto2.proto) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/EvilNamesProto2Kt.kt " +
          "$(location EvilNamesProto2Kt.kt) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/HardKeywordsAllTypesKt.kt " +
          "$(location HardKeywordsAllTypesProto2Kt.kt)",
    tools = ["//:protoc"],
)

genrule(
    name = "gen_evil_names_proto3_lite",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto3.proto"],
    outs = [
        "EvilNamesProto3LiteKt.kt",
        "HardKeywordsAllTypesProto3LiteKt.kt",
    ],
    cmd = "$(location //:protoc) " +
          "--kotlin_out=lite:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/evil_names_proto3.proto) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/EvilNamesProto3Kt.kt " +
          "$(location EvilNamesProto3LiteKt.kt) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/HardKeywordsAllTypesKt.kt " +
          "$(location HardKeywordsAllTypesProto3LiteKt.kt)",
    tools = ["//:protoc"],
)

genrule(
    name = "gen_evil_names_proto3",
    srcs = ["src/test/proto/com/google/protobuf/evil_names_proto3.proto"],
    outs = [
        "EvilNamesProto3Kt.kt",
        "HardKeywordsAllTypesProto3Kt.kt",
    ],
    cmd = "$(location //:protoc) " +
          "--kotlin_out=immutable:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/evil_names_proto3.proto) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/EvilNamesProto3Kt.kt " +
          "$(location EvilNamesProto3Kt.kt) && " +
          "cp $(@D)/src/test/proto/com/google/protobuf/HardKeywordsAllTypesKt.kt " +
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
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        ":shared_runtime",
        "//:java_lite_test_protos",
    ],
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
        "//:java_test_protos",
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
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        ":shared_runtime",
        "//:java_lite_test_protos",
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

junit_tests(
    name = "proto2_lite_test",
    srcs = ["src/test/kotlin/com/google/protobuf/Proto2LiteTest.kt"],
    deps = [
        ":evil_names_proto2_java_proto_lite",
        ":kotlin_unittest_lite",
        ":shared_runtime",
        "//java/core:test_util_lite",
        "//:java_lite_test_protos",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

junit_tests(
    name = "proto2_test",
    srcs = ["src/test/kotlin/com/google/protobuf/Proto2Test.kt"],
    deps = [
        ":evil_names_proto2_java_proto",
        ":kotlin_unittest",
        ":shared_runtime",
        "//java/core:test_util",
        "//:java_test_protos",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

junit_tests(
    name = "proto3_lite_test",
    srcs = ["src/test/kotlin/com/google/protobuf/Proto3LiteTest.kt"],
    deps = [
        ":evil_names_proto3_java_proto_lite",
        ":kotlin_proto3_unittest_lite",
        ":multiple_files_proto3_java_proto_lite",
        ":shared_runtime",
        "//:java_lite_test_protos",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)

junit_tests(
    name = "proto3_test",
    srcs = ["src/test/kotlin/com/google/protobuf/Proto3Test.kt"],
    deps = [
        ":evil_names_proto3_java_proto",
        ":kotlin_proto3_unittest",
        ":multiple_files_proto3_java_proto",
        ":shared_runtime",
        "//:java_test_protos",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
    ],
)
