load("@bazel_skylib//rules:build_test.bzl", "build_test")
load("@rules_java//java:defs.bzl", "java_library", "java_proto_library", "java_lite_proto_library")
load("@rules_jvm_external//:defs.bzl", "java_export")
load("@rules_proto//proto:defs.bzl", "proto_lang_toolchain", "proto_library")
load("//:internal.bzl", "conformance_test")
load("//:protobuf_version.bzl", "PROTOBUF_VERSION")
load("//java/internal:testing.bzl", "junit_tests")

LITE_SRCS = [
    # Keep in sync with `//java/lite:pom.xml`.
    "src/main/java/com/google/protobuf/AbstractMessageLite.java",
    "src/main/java/com/google/protobuf/AbstractParser.java",
    "src/main/java/com/google/protobuf/AbstractProtobufList.java",
    "src/main/java/com/google/protobuf/AllocatedBuffer.java",
    "src/main/java/com/google/protobuf/Android.java",
    "src/main/java/com/google/protobuf/ArrayDecoders.java",
    "src/main/java/com/google/protobuf/BinaryReader.java",
    "src/main/java/com/google/protobuf/BinaryWriter.java",
    "src/main/java/com/google/protobuf/BooleanArrayList.java",
    "src/main/java/com/google/protobuf/BufferAllocator.java",
    "src/main/java/com/google/protobuf/ByteBufferWriter.java",
    "src/main/java/com/google/protobuf/ByteOutput.java",
    "src/main/java/com/google/protobuf/ByteString.java",
    "src/main/java/com/google/protobuf/CodedInputStream.java",
    "src/main/java/com/google/protobuf/CodedInputStreamReader.java",
    "src/main/java/com/google/protobuf/CodedOutputStream.java",
    "src/main/java/com/google/protobuf/CodedOutputStreamWriter.java",
    "src/main/java/com/google/protobuf/DoubleArrayList.java",
    "src/main/java/com/google/protobuf/ExperimentalApi.java",
    "src/main/java/com/google/protobuf/ExtensionLite.java",
    "src/main/java/com/google/protobuf/ExtensionRegistryFactory.java",
    "src/main/java/com/google/protobuf/ExtensionRegistryLite.java",
    "src/main/java/com/google/protobuf/ExtensionSchema.java",
    "src/main/java/com/google/protobuf/ExtensionSchemaLite.java",
    "src/main/java/com/google/protobuf/ExtensionSchemas.java",
    "src/main/java/com/google/protobuf/FieldInfo.java",
    "src/main/java/com/google/protobuf/FieldSet.java",
    "src/main/java/com/google/protobuf/FieldType.java",
    "src/main/java/com/google/protobuf/FloatArrayList.java",
    "src/main/java/com/google/protobuf/GeneratedMessageInfoFactory.java",
    "src/main/java/com/google/protobuf/GeneratedMessageLite.java",
    "src/main/java/com/google/protobuf/IntArrayList.java",
    "src/main/java/com/google/protobuf/Internal.java",
    "src/main/java/com/google/protobuf/InvalidProtocolBufferException.java",
    "src/main/java/com/google/protobuf/IterableByteBufferInputStream.java",
    "src/main/java/com/google/protobuf/JavaType.java",
    "src/main/java/com/google/protobuf/LazyField.java",
    "src/main/java/com/google/protobuf/LazyFieldLite.java",
    "src/main/java/com/google/protobuf/LazyStringArrayList.java",
    "src/main/java/com/google/protobuf/LazyStringList.java",
    "src/main/java/com/google/protobuf/ListFieldSchema.java",
    "src/main/java/com/google/protobuf/LongArrayList.java",
    "src/main/java/com/google/protobuf/ManifestSchemaFactory.java",
    "src/main/java/com/google/protobuf/MapEntryLite.java",
    "src/main/java/com/google/protobuf/MapFieldLite.java",
    "src/main/java/com/google/protobuf/MapFieldSchema.java",
    "src/main/java/com/google/protobuf/MapFieldSchemaLite.java",
    "src/main/java/com/google/protobuf/MapFieldSchemas.java",
    "src/main/java/com/google/protobuf/MessageInfo.java",
    "src/main/java/com/google/protobuf/MessageInfoFactory.java",
    "src/main/java/com/google/protobuf/MessageLite.java",
    "src/main/java/com/google/protobuf/MessageLiteOrBuilder.java",
    "src/main/java/com/google/protobuf/MessageLiteToString.java",
    "src/main/java/com/google/protobuf/MessageSchema.java",
    "src/main/java/com/google/protobuf/MessageSetSchema.java",
    "src/main/java/com/google/protobuf/MutabilityOracle.java",
    "src/main/java/com/google/protobuf/NewInstanceSchema.java",
    "src/main/java/com/google/protobuf/NewInstanceSchemaLite.java",
    "src/main/java/com/google/protobuf/NewInstanceSchemas.java",
    "src/main/java/com/google/protobuf/NioByteString.java",
    "src/main/java/com/google/protobuf/OneofInfo.java",
    "src/main/java/com/google/protobuf/Parser.java",
    "src/main/java/com/google/protobuf/PrimitiveNonBoxingCollection.java",
    "src/main/java/com/google/protobuf/ProtoSyntax.java",
    "src/main/java/com/google/protobuf/Protobuf.java",
    "src/main/java/com/google/protobuf/ProtobufArrayList.java",
    "src/main/java/com/google/protobuf/ProtobufLists.java",
    "src/main/java/com/google/protobuf/ProtocolStringList.java",
    "src/main/java/com/google/protobuf/RawMessageInfo.java",
    "src/main/java/com/google/protobuf/Reader.java",
    "src/main/java/com/google/protobuf/RopeByteString.java",
    "src/main/java/com/google/protobuf/Schema.java",
    "src/main/java/com/google/protobuf/SchemaFactory.java",
    "src/main/java/com/google/protobuf/SchemaUtil.java",
    "src/main/java/com/google/protobuf/SmallSortedMap.java",
    "src/main/java/com/google/protobuf/StructuralMessageInfo.java",
    "src/main/java/com/google/protobuf/TextFormatEscaper.java",
    "src/main/java/com/google/protobuf/UninitializedMessageException.java",
    "src/main/java/com/google/protobuf/UnknownFieldSchema.java",
    "src/main/java/com/google/protobuf/UnknownFieldSetLite.java",
    "src/main/java/com/google/protobuf/UnknownFieldSetLiteSchema.java",
    "src/main/java/com/google/protobuf/UnmodifiableLazyStringList.java",
    "src/main/java/com/google/protobuf/UnsafeByteOperations.java",
    "src/main/java/com/google/protobuf/UnsafeUtil.java",
    "src/main/java/com/google/protobuf/Utf8.java",
    "src/main/java/com/google/protobuf/WireFormat.java",
    "src/main/java/com/google/protobuf/Writer.java",
]

# Should be used as `//java/lite`.
java_export(
    name = "lite",
    maven_coordinates = "com.google.protobuf:protobuf-javalite:%s" % PROTOBUF_VERSION,
    pom_template = "//java/lite:pom_template.xml",
    srcs = LITE_SRCS + [
        "//:gen_well_known_protos_javalite"
    ],
    visibility = [
        "//java/lite:__pkg__",
    ],
    resources = [
        "//:lite_well_known_protos",
    ],
)

java_library(
    name = "lite_runtime_only",
    srcs = LITE_SRCS,
)

java_export(
    name = "core",
    maven_coordinates = "com.google.protobuf:protobuf-java:%s" % PROTOBUF_VERSION,
    pom_template = "pom_template.xml",
    srcs = glob(
        [
            "src/main/java/com/google/protobuf/*.java",
        ],
        exclude = LITE_SRCS,
    ) + [
        "//:gen_well_known_protos_java",
    ],
    visibility = ["//visibility:public"],
    exports = [
        ":lite_runtime_only",
    ],
    deps = [
        ":lite_runtime_only",
    ],
    resources = [
        "//:well_known_protos",
    ],
)

filegroup(
    name = "release",
    visibility = ["//java:__pkg__"],
    srcs = [
        ":core-pom",
        ":core-maven-source",
        ":core-docs",
        ":core-project",
        ":lite-pom",
        ":lite-maven-source",
        ":lite-docs",
        ":lite-project",
    ]
)

proto_lang_toolchain(
    name = "toolchain",
    command_line = "--java_out=$(OUT)",
    runtime = ":core",
    visibility = ["//visibility:public"],
)

proto_library(
    name = "java_test_protos",
    srcs = glob(["src/test/proto/**/*.proto"]),
    strip_import_prefix = "src/test/proto",
    deps = [
        "//:any_proto",
        "//:descriptor_proto",
        "//:generic_test_protos",
        "//:wrappers_proto",
    ],
)

java_proto_library(
    name = "generic_test_protos_java_proto",
    visibility = [
        "//java:__subpackages__",
    ],
    deps = ["//:generic_test_protos"],
)

java_proto_library(
    name = "java_test_protos_java_proto",
    deps = [":java_test_protos"],
)

java_library(
    name = "test_util",
    srcs = [
        "src/test/java/com/google/protobuf/TestUtil.java",
        "src/test/java/com/google/protobuf/TestUtilLite.java"
    ],
    deps = [
        ":core",
        ":generic_test_protos_java_proto",
        ":java_test_protos_java_proto",
        "//external:guava",
        "//external:junit",
    ],
)

test_suite(
    name = "tests",
    tests = [
        "core_build_test",
        "conformance_test",
        "core_tests",
    ],
)

build_test(
    name = "core_build_test",
    targets = [
        ":core",
    ],
)

conformance_test(
    name = "conformance_test",
    testee = "//:conformance_java",
    failure_list = "//:conformance/failure_list_java.txt",
    text_format_failure_list = "//:conformance/text_format_failure_list_java.txt",
)

junit_tests(
    name = "core_tests",
    srcs = glob(["src/test/java/**/*.java"], exclude = [
        "src/test/java/com/google/protobuf/TestUtil.java",
        "src/test/java/com/google/protobuf/TestUtilLite.java",
    ]),
    data = ["//:testdata"],
    size = "large",
    deps = [
        ":core",
        ":generic_test_protos_java_proto",
        ":java_test_protos_java_proto",
        ":test_util",
        "//external:easymock",
        "//external:easymock_classextension",
        "//external:junit",
        "//external:truth",
    ]
)

java_lite_proto_library(
    name = "generic_test_protos_java_proto_lite",
    visibility = [
        "//java/lite:__pkg__",
    ],
    deps = ["//:generic_test_protos"],
)

java_lite_proto_library(
    name = "java_test_protos_java_proto_lite",
    visibility = [
        "//java/lite:__pkg__",
    ],
    deps = [":java_test_protos"],
)

genrule(
    name = "rewrite_javalite_test_util",
    srcs = [
        "//java/lite:lite.awk",
        "src/test/java/com/google/protobuf/TestUtil.java"
    ],
    outs = ["TestUtil.java"],
    cmd = "awk -f $(location //java/lite:lite.awk) $(location src/test/java/com/google/protobuf/TestUtil.java) > $@"
)

java_library(
    name = "test_util_lite",
    srcs = [
        ":rewrite_javalite_test_util",
        "src/test/java/com/google/protobuf/TestUtilLite.java"
    ],
    visibility = [
        "//java/lite:__pkg__",
    ],
    deps = [
        ":generic_test_protos_java_proto_lite",
        ":java_test_protos_java_proto_lite",
        ":lite_runtime_only",
        "//external:guava",
        "//external:junit",
    ],
)

LITE_TEST_EXCLUSIONS = [
    # Keep in sync with //java/lite:pom.xml id=copy-test-source-files execution.
    "src/test/java/com/google/protobuf/AbstractMessageTest.java",
    "src/test/java/com/google/protobuf/AbstractProto2SchemaTest.java",
    "src/test/java/com/google/protobuf/AnyTest.java",
    "src/test/java/com/google/protobuf/CodedInputStreamTest.java",
    "src/test/java/com/google/protobuf/DeprecatedFieldTest.java",
    "src/test/java/com/google/protobuf/DescriptorsTest.java",
    "src/test/java/com/google/protobuf/DiscardUnknownFieldsTest.java",
    "src/test/java/com/google/protobuf/DynamicMessageTest.java",
    "src/test/java/com/google/protobuf/ExtensionRegistryFactoryTest.java",
    "src/test/java/com/google/protobuf/FieldPresenceTest.java",
    "src/test/java/com/google/protobuf/ForceFieldBuildersPreRun.java",
    "src/test/java/com/google/protobuf/GeneratedMessageTest.java",
    "src/test/java/com/google/protobuf/LazyFieldTest.java",
    "src/test/java/com/google/protobuf/LazyStringEndToEndTest.java",
    "src/test/java/com/google/protobuf/MapForProto2Test.java",
    "src/test/java/com/google/protobuf/MapTest.java",
    "src/test/java/com/google/protobuf/MessageTest.java",
    "src/test/java/com/google/protobuf/NestedBuildersTest.java",
    "src/test/java/com/google/protobuf/PackedFieldTest.java",
    "src/test/java/com/google/protobuf/ParserTest.java",
    "src/test/java/com/google/protobuf/ParseExceptionsTest.java",
    "src/test/java/com/google/protobuf/Proto2ExtensionLookupSchemaTest.java",
    "src/test/java/com/google/protobuf/Proto2SchemaTest.java",
    "src/test/java/com/google/protobuf/Proto2UnknownEnumValueTest.java",
    "src/test/java/com/google/protobuf/RepeatedFieldBuilderV3Test.java",
    "src/test/java/com/google/protobuf/ServiceTest.java",
    "src/test/java/com/google/protobuf/SingleFieldBuilderV3Test.java",
    "src/test/java/com/google/protobuf/TestBadIdentifiers.java",
    "src/test/java/com/google/protobuf/TextFormatParseInfoTreeTest.java",
    "src/test/java/com/google/protobuf/TextFormatParseLocationTest.java",
    "src/test/java/com/google/protobuf/TextFormatTest.java",
    "src/test/java/com/google/protobuf/TestUtil.java",
    "src/test/java/com/google/protobuf/TestUtilLite.java",
    "src/test/java/com/google/protobuf/TypeRegistryTest.java",
    "src/test/java/com/google/protobuf/UnknownEnumValueTest.java",
    "src/test/java/com/google/protobuf/UnknownFieldSetLiteTest.java",
    "src/test/java/com/google/protobuf/UnknownFieldSetTest.java",
    "src/test/java/com/google/protobuf/WellKnownTypesTest.java",
    "src/test/java/com/google/protobuf/WireFormatTest.java",
]

junit_tests(
    name = "lite_tests",
    srcs = glob(["src/test/java/**/*.java"], exclude = LITE_TEST_EXCLUSIONS),
    data = ["//:testdata"],
    test_prefix = "Lite",
    size = "large",
    deps = [
        ":lite",
        ":generic_test_protos_java_proto_lite",
        ":java_test_protos_java_proto_lite",
        ":test_util_lite",
        "//external:easymock",
        "//external:easymock_classextension",
        "//external:junit",
        "//external:truth",
    ]
)

