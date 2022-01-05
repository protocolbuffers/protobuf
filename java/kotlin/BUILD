load("@io_bazel_rules_kotlin//kotlin:jvm.bzl", "kt_jvm_library", "kt_jvm_test")
load("@rules_java//java:defs.bzl", "java_proto_library")
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
        "src/main/kotlin/com/google/protobuf/UnmodifiableCollections.kt",
    ],
    visibility = ["//visibility:public"],
    deps = [
        ":only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/lite:lite",
    ],
)

kt_jvm_library(
    name = "only_for_use_in_proto_generated_code_its_generator_and_tests",
    srcs = ["src/main/kotlin/com/google/protobuf/OnlyForUseByGeneratedProtoCode.kt"],
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

java_proto_library(
    name = "evil_names_proto2_java_proto",
    deps = [":evil_names_proto2"],
)

java_proto_library(
    name = "evil_names_proto3_java_proto",
    deps = [":evil_names_proto3"],
)

proto_library(
    name = "multiple_files_proto3",
    srcs = ["src/test/proto/com/google/protobuf/multiple_files_proto3.proto"],
)

java_proto_library(
    name = "multiple_files_proto3_java_proto",
    deps = [":multiple_files_proto3"],
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
          "--kotlin_out=shared,immutable:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/multiple_files_proto3.proto) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/MultipleFilesMessageAKt.kt " +
          "$(location MultipleFilesMessageAKt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/MultipleFilesMessageBKt.kt " +
          "$(location MultipleFilesMessageBKt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/MultipleFilesProto3Kt.kt " +
          "$(location MultipleFilesProto3Kt.kt)",
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
          "--kotlin_out=shared,immutable:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/evil_names_proto2.proto) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/EvilNamesProto2Kt.kt " +
          "$(location EvilNamesProto2Kt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/HardKeywordsAllTypesProto2Kt.kt " +
          "$(location HardKeywordsAllTypesProto2Kt.kt)",
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
          "--kotlin_out=shared,immutable:$(@D) " +
          "$(location src/test/proto/com/google/protobuf/evil_names_proto3.proto) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/EvilNamesProto3Kt.kt " +
          "$(location EvilNamesProto3Kt.kt) && " +
          "cp $(@D)/com/google/protobuf/kotlin/generator/HardKeywordsAllTypesProto3Kt.kt " +
          "$(location HardKeywordsAllTypesProto3Kt.kt)",
    tools = ["//:protoc"],
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

kt_jvm_test(
    name = "proto2_test",
    srcs = ["src/test/kotlin/com/google/protobuf/Proto2Test.kt"],
    deps = [
        ":kotlin_unittest",
        "@maven//:com_google_truth_truth",
    ],
)
