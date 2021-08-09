load("@io_bazel_rules_kotlin//kotlin:kotlin.bzl", "kt_jvm_library")

KT_SRCS = [
    "src/main/kotlin/com/google/protobuf/DslList.kt",
    "src/main/kotlin/com/google/protobuf/DslMap.kt",
    "src/main/kotlin/com/google/protobuf/DslProxy.kt",
    "src/main/kotlin/com/google/protobuf/ExtendableMessageExtensions.kt",
    "src/main/kotlin/com/google/protobuf/ExtendableMessageLiteExtensions.kt",
    "src/main/kotlin/com/google/protobuf/ExtensionList.kt",
    "src/main/kotlin/com/google/protobuf/OnlyForUseByGeneratedProtoCode.kt",
    "src/main/kotlin/com/google/protobuf/ProtoDslMarker.kt",
    "src/main/kotlin/com/google/protobuf/UnmodifiableCollections.kt",
]

kt_jvm_library(
    name = "kt_runtime",
    srcs = KT_SRCS,
    deps = [
        "//java/core:java_runtime",
    ]
)
