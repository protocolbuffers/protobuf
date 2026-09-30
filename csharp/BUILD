# BUILD rules for C# core proto libraries

load("//bazel:cc_proto_library.bzl", "cc_proto_library")
load("//bazel:proto_library.bzl", "proto_library")
load("//editions:defaults.bzl", "compile_edition_defaults", "embed_edition_defaults")
load("//tools/build_defs/csharp/mono:build_defs.bzl", "mono_cs_test", "mono_roslyn_cs_library")

package(default_applicable_licenses = ["//:license"])

licenses(["notice"])

proto_library(
    name = "c_sharp_features_proto",
    srcs = ["google/protobuf/c_sharp_features.proto"],
    compatible_with = [
        "//buildenv/target:non_prod",
    ],
    visibility = ["//visibility:public"],
    deps = ["//:descriptor_proto"],
)

cc_proto_library(
    name = "c_sharp_features_cc_proto",
    compatible_with = [
        "//buildenv/target:non_prod",
    ],
    visibility = [
        "//editions:__pkg__",
        "//src/google/protobuf/compiler/csharp:__subpackages__",
    ],
    deps = [":c_sharp_features_proto"],
)

genrule(
    name = "descriptor_gen",
    srcs = ["//net/proto2/proto:descriptor_proto_source"],
    outs = ["src/Google.Protobuf/Reflection/Descriptor.pb.cs"],
    cmd = ("$(location //net/proto2/compiler/public:protocol_compiler) -I. " +
           "--csharp_out=$(RULEDIR)/src/Google.Protobuf " +
           "--csharp_opt=file_extension=.pb.cs " +
           "--csharp_opt=base_namespace=Google.Protobuf $<"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = ["//net/proto2/compiler/public:protocol_compiler"],
    visibility = ["//visibility:private"],
)

genrule(
    name = "features_gen",
    srcs = [
        "//net/proto2/proto:descriptor_proto_source",
        "google/protobuf/c_sharp_features.proto",
    ],
    outs = ["src/Google.Protobuf/Reflection/CSharpFeatures.pb.cs"],
    cmd = ("$(location //net/proto2/compiler/public:protocol_compiler) -I. " +
           "--csharp_out=$(RULEDIR)/src/Google.Protobuf " +
           "--csharp_opt=file_extension=.pb.cs " +
           "--csharp_opt=base_namespace=Google.Protobuf $(SRCS)"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = ["//net/proto2/compiler/public:protocol_compiler"],
    visibility = ["//visibility:private"],
)

genrule(
    name = "wkt_gen",
    srcs = [
        "//:any_proto.proto",
        "//:api_proto.proto",
        "//:duration_proto.proto",
        "//:empty_proto.proto",
        "//:field_mask_proto.proto",
        "//:source_context_proto.proto",
        "//:struct_proto.proto",
        "//:timestamp_proto.proto",
        "//:type_proto.proto",
        "//:wrappers_proto.proto",
    ],
    outs = [
        "src/Google.Protobuf/WellKnownTypes/Any.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/Api.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/Duration.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/Empty.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/FieldMask.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/SourceContext.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/Struct.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/Timestamp.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/Type.pb.cs",
        "src/Google.Protobuf/WellKnownTypes/Wrappers.pb.cs",
    ],
    cmd = ("$(location //net/proto2/compiler/public:protocol_compiler) -I. " +
           "--csharp_out=$(RULEDIR)/src/Google.Protobuf " +
           "--csharp_opt=file_extension=.pb.cs " +
           "--csharp_opt=base_namespace=Google.Protobuf $(SRCS)"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = ["//net/proto2/compiler/public:protocol_compiler"],
    visibility = ["//visibility:private"],
)

# Note that the open source Google.Protobuf.Compiler namespace does not include
# protos from Profile, as that's google3-only.
genrule(
    name = "plugin_gen",
    srcs = [
        "//net/proto2/proto:descriptor_proto_source",
        "//net/proto2/compiler/proto:profile.proto",
        "//src/google/protobuf/compiler:plugin_proto_source",
    ],
    outs = [
        "src/Google.Protobuf/Compiler/Plugin.pb.cs",
        "src/Google.Protobuf/Compiler/Profile.pb.cs",
    ],
    cmd = ("$(location //net/proto2/compiler/public:protocol_compiler) -I. " +
           "--csharp_out=$(RULEDIR)/src/Google.Protobuf " +
           "--csharp_opt=file_extension=.pb.cs " +
           "--csharp_opt=base_namespace=Google.Protobuf $(SRCS)"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = ["//net/proto2/compiler/public:protocol_compiler"],
    visibility = ["//visibility:private"],
)

genrule(
    name = "addressbook_gen",
    srcs = [
        "///examples:addressbook.proto",
        "//:timestamp_proto.proto",
    ],
    outs = [
        "src/AddressBook/Addressbook.pb.cs",
    ],
    cmd = ("$(location //net/proto2/compiler/public:protocol_compiler) " +
           "-I$$(dirname $(location ///examples:addressbook.proto)) " +
           "-I. " +
           "--csharp_out=$(RULEDIR)/src/AddressBook " +
           "--csharp_opt=file_extension=.pb.cs $(SRCS)"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = ["//net/proto2/compiler/public:protocol_compiler"],
    visibility = ["//visibility:private"],
)

genrule(
    name = "conformance_gen",
    testonly = True,
    srcs = [
        "//net/proto2/proto:descriptor_proto_source",
        "//src/google/protobuf:json_enumvalue_options.proto",
        "//conformance:conformance.proto",
        "//conformance/test_protos:test_messages_edition2023.proto",
        "//conformance/test_protos:test_messages_edition2026.proto",
        "//conformance/test_protos:test_messages_edition_unstable.proto",
    ],
    outs = [
        "src/Google.Protobuf.Conformance/Conformance.pb.cs",
        "src/Google.Protobuf.Conformance/TestMessagesEdition2023.pb.cs",
        "src/Google.Protobuf.Conformance/TestMessagesEdition2026.pb.cs",
        "src/Google.Protobuf.Conformance/TestMessagesEditionUnstable.pb.cs",
    ],
    cmd = ("$(execpath //net/proto2/compiler/public:protocol_compiler) " +
           "-I. " +
           "-I$$(dirname $(execpath //conformance:conformance.proto)) " +
           "--csharp_out=$(RULEDIR)/src/Google.Protobuf.Conformance " +
           "--csharp_opt=file_extension=.pb.cs " +
           "$(execpath //conformance:conformance.proto) " +
           "$(execpath //conformance/test_protos:test_messages_edition2023.proto) " +
           "$(execpath //conformance/test_protos:test_messages_edition2026.proto) " +
           "$(execpath //conformance/test_protos:test_messages_edition_unstable.proto)"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = ["//net/proto2/compiler/public:protocol_compiler"],
    visibility = ["//visibility:private"],
)

genrule(
    name = "test_gen",
    testonly = True,
    srcs = [
        "//:struct_proto.proto",
        "//:any_proto.proto",
        "//:api_proto.proto",
        "//:duration_proto.proto",
        "//:empty_proto.proto",
        "//:field_mask_proto.proto",
        "//:source_context_proto.proto",
        "//:timestamp_proto.proto",
        "//:type_proto.proto",
        "//:wrappers_proto.proto",
        "//net/proto2/proto:descriptor_proto_source",
        "protos/map_unittest_proto3.proto",
        "protos/nrt.proto",
        "protos/unittest_proto3.proto",
        "protos/unittest_import_proto3.proto",
        "protos/unittest_import_public_proto3.proto",
        "protos/unittest_issues.proto",
        "protos/unittest_custom_options_proto3.proto",
        "protos/unittest.proto",
        "protos/unittest_import.proto",
        "protos/unittest_import_public.proto",
        "protos/unittest_issue6936_a.proto",
        "protos/unittest_issue6936_b.proto",
        "protos/unittest_issue6936_c.proto",
        "protos/unittest_selfreferential_options.proto",
        "google/protobuf/c_sharp_features.proto",
        "//java/core:java_features_proto_source",
        "//:src/google/protobuf/unittest_well_known_types.proto",
        "//src/google/protobuf:test_messages_proto2.proto",
        "//src/google/protobuf:test_messages_proto3.proto",
        "//src/google/protobuf:unittest_features.proto",
        "//src/google/protobuf:unittest_legacy_features.proto",
        "//src/google/protobuf:unittest_proto3_optional.proto",
        "//src/google/protobuf:unittest_retention.proto",
        "//editions:golden/test_messages_proto2_editions.proto",
        "//editions:golden/test_messages_proto3_editions.proto",
    ],
    outs = [
        "src/Google.Protobuf.Test/testprotos.pb",
        "src/Google.Protobuf.Test.TestProtos/MapUnittestProto3.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/Nrt.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestIssue6936B.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/TestMessagesProto2.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestIssue6936C.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/TestMessagesProto3.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestIssues.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestCustomOptionsProto3.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestFeatures.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestLegacyFeatures.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/Unittest.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestImport.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestProto3Optional.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestImportProto3.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestProto3.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestImportPublic.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestSelfreferentialOptions.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestImportPublicProto3.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestWellKnownTypes.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestIssue6936A.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/UnittestRetention.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/TestMessagesProto2Editions.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/TestMessagesProto3Editions.pb.cs",
        "src/Google.Protobuf.Test.TestProtos/JavaFeatures.pb.cs",
    ],
    cmd = ("$(location //net/proto2/compiler/public:protocol_compiler) " +
           "$(SRCS) " +
           "-Ithird_party/protobuf " +
           "-Ithird_party/protobuf/editions " +
           "-I. " +
           "--csharp_out=$(RULEDIR)/src/Google.Protobuf.Test.TestProtos " +
           "--csharp_opt=file_extension=.pb.cs " +
           "--include_source_info " +
           "--include_imports " +
           "--descriptor_set_out=$(RULEDIR)/src/Google.Protobuf.Test/testprotos.pb " +
           "--experimental_editions"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = [
        "//net/proto2/compiler/public:protocol_compiler",
    ],
    visibility = ["//visibility:private"],
)

genrule(
    name = "json_options_test_gen",
    testonly = True,
    srcs = [
        "//net/proto2/proto:descriptor_proto_source",
        "//src/google/protobuf:json_enumvalue_options.proto",
        "//src/google/protobuf:json_options.proto",
        "//src/google/protobuf/json:json_enumval_custom_string_proto_srcs",
    ],
    outs = [
        "src/Google.Protobuf.Test.TestProtos/JsonEnumvalCustomString.pb.cs",
    ],
    cmd = ("$(location //net/proto2/compiler/public:protocol_compiler) " +
           "-I. " +
           "--csharp_out=$(RULEDIR)/src/Google.Protobuf.Test.TestProtos " +
           "--csharp_opt=file_extension=.pb.cs " +
           "$(location " +
           "//src/google/protobuf/json:json_enumval_custom_string_proto_srcs)"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = ["//net/proto2/compiler/public:protocol_compiler"],
    visibility = ["//visibility:private"],
)

compile_edition_defaults(
    name = "csharp_edition_defaults",
    srcs = [
        "//:descriptor_proto",
    ],
    compatible_with = ["//buildenv/target:non_prod"],
    maximum_edition = "2026",
    minimum_edition = "PROTO2",
)

embed_edition_defaults(
    name = "embedded_csharp_edition_defaults_generate",
    compatible_with = ["//buildenv/target:non_prod"],
    defaults = "csharp_edition_defaults",
    encoding = "base64",
    output = "src/Google.Protobuf/Reflection/FeatureSetDescriptor.g.cs",
    placeholder = "DEFAULTS_VALUE",
    template = "src/Google.Protobuf/Reflection/FeatureSetDescriptor.g.cs.template",
)

genrule(
    name = "json_options_gen",
    srcs = [
        "//net/proto2/proto:descriptor_proto_source",
        "//src/google/protobuf:json_enumvalue_options.proto",
        "//src/google/protobuf:json_options.proto",
    ],
    outs = [
        "src/Google.Protobuf/JsonOptions.pb.cs",
        "src/Google.Protobuf/JsonEnumvalueOptions.pb.cs",
    ],
    cmd = ("$(location //net/proto2/compiler/public:protocol_compiler) -I. " +
           "--csharp_out=$(RULEDIR)/src/Google.Protobuf " +
           "--csharp_opt=file_extension=.pb.cs " +
           "$(location //src/google/protobuf:json_options.proto) " +
           "$(location //src/google/protobuf:json_enumvalue_options.proto)"),
    compatible_with = ["//buildenv/target:non_prod"],
    tools = ["//net/proto2/compiler/public:protocol_compiler"],
    visibility = ["//visibility:private"],
)

# TODO: Consider renaming this to Google.Protobuf to match the NuGet package name,
# *or* rename all targets to be more Blaze-idiomatic... but we need the assemblies themselves
# to match the normal names so that InternalsVisibleTo works.
mono_roslyn_cs_library(
    name = "GoogleProtobuf",
    srcs = [
        ":descriptor_gen",
        ":embedded_csharp_edition_defaults_generate",
        ":features_gen",
        ":json_options_gen",
        ":plugin_gen",
        ":wkt_gen",
    ] + glob(
        [
            "src/Google.Protobuf/**/*.cs",
        ],
    ),
    # TODO: Reinstate this and remove the SIGNING_DISABLED part.
    # This requires a signed version of System.Runtime.CompilerServices.Unsafe
    # keyfile = "keys/Google.Protobuf.snk",
    flags = ["-define:SIGNING_DISABLED"],
    unsafe = True,
    visibility = ["//tools/build_defs/csharp/mono:visibility"],
    deps = [
        "//third_party/mono:System.Runtime",
        "//third_party/mono:System.Runtime.CompilerServices.Unsafe",
    ],
)

mono_roslyn_cs_library(
    name = "Google.Protobuf.Test.TestProtos",
    testonly = True,
    srcs = [
        ":json_options_test_gen",
        ":test_gen",
        "//src/google/protobuf/csharp/protos/unittest_deep_dependencies:cached_csharp_gen",
        "//src/google/protobuf/csharp/protos/unittest_deep_dependencies:notcached_csharp_gen",
    ] + glob(
        ["src/Google.Protobuf.Test.TestProtos/**/*.cs"],
    ),
    visibility = ["//visibility:private"],
    deps = [
        ":GoogleProtobuf",
    ],
)

mono_cs_test(
    name = "Google.Protobuf.Test",
    size = "small",
    testonly = True,
    srcs = glob(["src/Google.Protobuf.Test/**/*.cs"]),
    # keyfile = "keys/Google.Protobuf.snk",
    resources = {
        "src/Google.Protobuf.Test/testprotos.pb": "Google.Protobuf.Test.testprotos.pb",
    },
    unsafe = True,
    visibility = ["//visibility:private"],
    deps = [
        ":Google.Protobuf.Test.TestProtos",
        ":GoogleProtobuf",
        "//third_party/dotnet_src/nunit:nunit.framework",
    ],
)

# This target exists primarily to ensure that we have not made any breaking changes.
mono_roslyn_cs_library(
    name = "AddressBook",
    srcs = [
        ":addressbook_gen",
    ] + glob(["src/AddressBook/*.cs"]),
    deps = [
        ":GoogleProtobuf",
    ],
)

# This target exists primarily to ensure that we have not made any breaking changes.
# This does *not* run the actual conformance tests.
mono_roslyn_cs_library(
    name = "Google.Protobuf.Conformance",
    testonly = True,
    srcs = [
        "src/Google.Protobuf.Conformance/Program.cs",
        ":conformance_gen",
    ],
    deps = [
        ":Google.Protobuf.Test.TestProtos",
        ":GoogleProtobuf",
    ],
)

filegroup(
    name = "protobuf_support_sources",
    srcs = [
        ":descriptor_gen",
        ":embedded_csharp_edition_defaults_generate",
        ":json_options_gen",
        ":wkt_gen",
    ] + glob(
        [
            "src/Google.Protobuf/**/*.cs",
        ],
    ),
    compatible_with = ["//buildenv/target:non_prod"],
    visibility = ["//learning/deepmind/rl/environments/dmworlds/unity:__subpackages__"],
)

filegroup(
    name = "c_sharp_features_proto_srcs",
    srcs = ["google/protobuf/c_sharp_features.proto"],
    compatible_with = ["//buildenv/target:non_prod"],
    visibility = ["//:__subpackages__"],
)
