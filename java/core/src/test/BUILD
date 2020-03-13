load("@rules_java//java:defs.bzl", "java_library", "java_lite_proto_library", "java_proto_library", "java_test")
load("@rules_proto//proto:defs.bzl", "proto_library")

proto_library(
    name = "test_protos",
    srcs = glob(
        [
            "proto/**/*.proto",
        ],
        exclude = [
            # Uses unknown option `experimental_java_message_interface`.
            "proto/com/google/protobuf/test_extra_interfaces.proto",
        ],
    ),
    strip_import_prefix = "proto",
    deps = [
        "//:any_proto",
        "//:descriptor_proto",
        "//:unittest_custom_options_proto",
        "//:unittest_proto",
        "//:unittest_proto3_proto",
        "//:wrappers_proto",
    ],
)

java_proto_library(
    name = "test_java_protos",
    deps = [
        ":test_protos",
    ],
)

java_lite_proto_library(
    name = "test_java_lite_protos",
    visibility = [
        "//java/lite:__subpackages__",
    ],
    deps = [
        ":test_protos",
    ],
)

java_library(
    name = "test_util_lite",
    srcs = [
        "java/com/google/protobuf/TestUtilLite.java",
    ],
    visibility = [
        "//java/lite:__subpackages__",
    ],
    deps = [
        "//:unittest_import_lite_java_proto",
        "//:unittest_import_public_lite_java_proto",
        "//:unittest_lite_java_proto",
        "//:unittest_proto3_optional_java_proto",
        "//java/lite",
    ],
)

java_library(
    name = "test",
    srcs = glob(
        [
            "java/**/*.java",
        ],
        exclude = [
            "java/com/google/protobuf/TestUtilLite.java",
        ],
    ),
    data = [
        "//src/google/protobuf/testdata",
    ],
    deps = [
        ":test_java_protos",
        ":test_util_lite",
        "//:any_java_proto",
        "//:unittest_custom_options_java_proto",
        "//:unittest_enormous_descriptor_java_proto",
        "//:unittest_import_java_proto",
        "//:unittest_import_lite_java_proto",
        "//:unittest_import_public_java_proto",
        "//:unittest_import_public_lite_java_proto",
        "//:unittest_java_proto",
        "//:unittest_lite_java_proto",
        "//:unittest_mset_java_proto",
        "//:unittest_mset_wire_format_java_proto",
        "//:unittest_no_generic_services_java_proto",
        "//:unittest_optimize_for_java_proto",
        "//:unittest_proto3_java_proto",
        "//:unittest_proto3_optional_java_proto",
        "//:unittest_well_known_types_java_proto",
        "//java/core",
        "//java/lite",
        "@maven//:com_google_guava_guava",
        "@maven//:com_google_truth_truth",
        "@maven//:junit_junit",
        "@maven//:org_easymock_easymock",
        "@maven//:org_easymock_easymockclassextension",
    ],
)

[
    java_test(
        name = src[:-len(".java")],
        runtime_deps = [
            ":test",
        ],
    )
    for src in glob(
        [
            "java/**/*Test.java",
        ],
        exclude = [
            "**/Abstract*Test.java",
            "java/com/google/protobuf/CodedOutputStreamTest.java",
            "java/com/google/protobuf/ExtensionRegistryFactoryTest.java",
        ],
    )
]
