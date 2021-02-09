# Tests for the Kotlin proto runtime.
load("//tools/build_defs/android:rules.bzl", "android_binary")
load("//tools/build_defs/kotlin:rules.bzl", "kt_jvm_library")
load("//testing/build_defs:junit_test_suites.bzl", "junit_test_suites")
load(
    "//third_party/protobuf/build_defs:kt_jvm_proto_library.bzl",
    "kt_jvm_lite_proto_library",
    "kt_jvm_proto_library",
)

proto_library(
    name = "test_proto",
    srcs = ["test.proto"],
)

java_proto_library(
    name = "test_java_proto",
    deps = [":test_proto"],
)

java_lite_proto_library(
    name = "test_java_proto_lite",
    deps = [":test_proto"],
)

kt_jvm_library(
    name = "shared_tests",
    srcs = [
        "DslListTest.kt",
        "DslMapTest.kt",
        "ExtensionListTest.kt",
    ],
    kotlincopts = ["-Xopt-in=kotlin.RequiresOptIn"],
    deps = [
        ":test_java_proto_lite",
        "//java/com/google/common/testing",
        "//java/com/google/protobuf/kotlin",
        "//java/com/google/protobuf/kotlin:only_for_use_in_proto_generated_code_its_generator_and_tests",
        "//java/com/google/protobuf/kotlin:shared_runtime",
        "//third_party/java/junit",
        "//third_party/java/truth",
        "//third_party/kotlin/kotlin:kotlin_test",
    ],
)

junit_test_suites(
    name = "shared_test_suite",
    sizes = ["small"],
    suffix = "Shared",
    deps = [":shared_tests"],
)

proto_library(
    name = "evil_names_proto2",
    srcs = ["evil_names_proto2.proto"],
)

proto_library(
    name = "evil_names_proto3",
    srcs = ["evil_names_proto3.proto"],
)

proto_library(
    name = "multiple_files_proto3",
    srcs = ["multiple_files_proto3.proto"],
)

kt_jvm_lite_proto_library(
    name = "proto2_unittest_kt_jvm_proto_lite",
    deps = [
        ":evil_names_proto2",
        "//net/proto2/internal:unittest_lite_proto",
    ],
)

kt_jvm_proto_library(
    name = "proto2_unittest_kt_jvm_proto",
    deps = [
        ":evil_names_proto2",
        "//net/proto2/internal:unittest_proto",
    ],
)

kt_jvm_lite_proto_library(
    name = "proto3_unittest_kt_jvm_proto_lite",
    deps = [
        ":evil_names_proto3",
        ":multiple_files_proto3",
        "//net/proto2/internal:unittest_proto3",
    ],
)

kt_jvm_proto_library(
    name = "proto3_unittest_kt_jvm_proto",
    deps = [
        ":evil_names_proto3",
        ":multiple_files_proto3",
        "//net/proto2/internal:unittest_proto3",
    ],
)

kt_jvm_library(
    name = "proto2_lite_test",
    srcs = ["Proto2LiteTest.kt"],
    constraints = ["android"],
    kotlincopts = [
        "-language-version",
        "1.3",
    ],
    deps = [
        ":proto2_unittest_kt_jvm_proto_lite",
        "//java/com/google/protobuf/kotlin:shared_runtime",
        "//javatests/com/google/protobuf:test_util-android",
        "//third_party/java/junit:junit-android",
        "//third_party/java/truth:truth-android",
        "//third_party/kotlin/kotlin:kotlin_test",
    ],
)

kt_jvm_library(
    name = "proto2_test",
    srcs = ["Proto2Test.kt"],
    kotlincopts = [
        "-language-version",
        "1.3",
    ],
    deps = [
        "proto2_unittest_kt_jvm_proto",
        "//java/com/google/protobuf/kotlin:shared_runtime",
        "//javatests/com/google/protobuf:test_util",
        "//net/proto2/internal:unittest_java_proto",
        "//third_party/java/junit",
        "//third_party/java/truth",
        "//third_party/kotlin/kotlin:kotlin_test",
    ],
)

kt_jvm_library(
    name = "proto3_lite_test",
    srcs = ["Proto3Test.kt"],
    kotlincopts = [
        "-language-version",
        "1.3",
    ],
    deps = [
        ":proto3_unittest_kt_jvm_proto_lite",
        "//java/com/google/protobuf/kotlin:shared_runtime",
        "//net/proto2/internal:unittest_proto3_java_proto_lite",
        "//third_party/java/junit",
        "//third_party/java/truth",
        "//third_party/kotlin/kotlin:kotlin_test",
    ],
)

kt_jvm_library(
    name = "proto3_test",
    srcs = ["Proto3Test.kt"],
    kotlincopts = [
        "-language-version",
        "1.3",
    ],
    deps = [
        ":proto3_unittest_kt_jvm_proto",
        "//java/com/google/protobuf/kotlin:shared_runtime",
        "//net/proto2/internal:unittest_proto3_java_proto",
        "//third_party/java/junit",
        "//third_party/java/truth",
        "//third_party/kotlin/kotlin:kotlin_test",
    ],
)

junit_test_suites(
    name = "proto2_lite_tests_junit",
    sizes = ["small"],
    suffix = "Proto2LiteGeneratedCode",
    deps = [":proto2_lite_test"],
)

junit_test_suites(
    name = "proto2_tests_junit",
    sizes = ["small"],
    suffix = "Proto2GeneratedCode",
    deps = [":proto2_test"],
)

junit_test_suites(
    name = "generated_proto3_lite",
    sizes = ["small"],
    suffix = "Proto3LiteGeneratedCode",
    deps = ["proto3_lite_test"],
)

junit_test_suites(
    name = "generated_full_protos",
    sizes = ["small"],
    suffix = "Proto3GeneratedCode",
    deps = [":proto3_test"],
)

kt_jvm_library(
    name = "lite_runtime_tests_lib",
    srcs = ["ExtendableMessageLiteExtensionsTest.kt"],
    constraints = ["android"],
    deps = [
        ":test_java_proto_lite",
        "//java/com/google/protobuf/kotlin:kotlin_lite",
        "//third_party/java/junit:junit-android",
        "//third_party/java/truth:truth-android",
    ],
)

junit_test_suites(
    name = "lite_runtime_tests",
    sizes = ["small"],
    suffix = "LiteRuntime",
    deps = [":lite_runtime_tests_lib"],
)

kt_jvm_library(
    name = "full_runtime_tests_lib",
    srcs = ["ExtendableMessageExtensionsTest.kt"],
    deps = [
        ":test_java_proto",
        "//java/com/google/protobuf/kotlin",
        "//third_party/java/junit",
        "//third_party/java/truth",
    ],
)

junit_test_suites(
    name = "full_runtime_tests",
    sizes = ["small"],
    suffix = "FullRuntime",
    deps = [":full_runtime_tests_lib"],
)

# Generate a binary with AppReduce run on it, to verify no choking.  The proto runtime includes
# kt_proto.pgcfg, which enforces that the DSLs are stripped out, so we just want to make sure
# that's run.
android_binary(
    name = "Proto2LiteTestBinary",
    manifest = "AndroidManifest.xml",
    proguard_specs = [
        "//java/com/google/android/apps/common/proguard:base.pgcfg",
        "//java/com/google/android/apps/common/proguard:dev_optimize.pgcfg",
    ],
    deps = [":proto2_lite_test"],
)
