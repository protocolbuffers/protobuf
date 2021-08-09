load("@io_bazel_rules_kotlin//kotlin:kotlin.bzl", "kt_jvm_library")

KT_LITE_SRCS = [
    "src/main/kotlin/com/google/protobuf/DslList.kt",
    "src/main/kotlin/com/google/protobuf/DslMap.kt",
    "src/main/kotlin/com/google/protobuf/DslProxy.kt",
    "src/main/kotlin/com/google/protobuf/ExtendableMessageLiteExtensions.kt",
    "src/main/kotlin/com/google/protobuf/ExtensionList.kt",
    "src/main/kotlin/com/google/protobuf/OnlyForUseByGeneratedProtoCode.kt",
    "src/main/kotlin/com/google/protobuf/ProtoDslMarker.kt",
    "src/main/kotlin/com/google/protobuf/UnmodifiableCollections.kt",
]

kt_jvm_library(
    name = "kt_lite",
    srcs = KT_LITE_SRCS,
    deps = [
        "//java/lite",
    ],
    visibility = ["//java/kotlin-lite:__pkg__"],
)

kt_jvm_library(
    name = "kt_core",
    srcs = glob(
        [
            "src/main/kotlin/com/google/protobuf/*.kt",
        ],
        exclude = KT_LITE_SRCS,
    ),
    deps = [
        ":kt_lite",
        "//java/core",
    ],
    visibility = ["//visibility:public"],
)
