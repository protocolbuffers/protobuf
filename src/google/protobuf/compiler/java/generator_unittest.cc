// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/compiler/java/generator.h"

#include <memory>
#include <string>

#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/testing/file.h"
#include "google/protobuf/descriptor.pb.h"
#include <gtest/gtest.h>
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "google/protobuf/compiler/java/java_features.pb.h"
#include "google/protobuf/compiler/command_line_interface_tester.h"


namespace google {
namespace protobuf {
namespace compiler {
namespace java {
namespace {

#define PACKAGE_PREFIX ""
#define PACKAGE_IMPORT_PREFIX ""

class JavaGeneratorTest : public CommandLineInterfaceTester {
 protected:
  JavaGeneratorTest() {
    RegisterGenerator("--java_out", "--java_opt",
                      std::make_unique<JavaGenerator>(), "Java test generator");

    // Generate built-in protos.
    CreateTempFile(
        "google/protobuf/descriptor.proto",
        google::protobuf::DescriptorProto::descriptor()->file()->DebugString());
    CreateTempFile("third_party/java/protobuf/java_features.proto",
                   pb::JavaFeatures::descriptor()->file()->DebugString());
  }

  bool FileGenerated(absl::string_view filename) {
    std::string path = absl::StrCat(temp_directory(), "/", filename);
    return File::Exists(path);
  }

  bool FileContainsSubstring(absl::string_view filename,
                             absl::string_view substring) {
    std::string path = absl::StrCat(temp_directory(), "/", filename);
    std::string contents;
    if (!File::GetContents(path, &contents, true).ok()) {
      return false;
    }
    return contents.find(substring) != std::string::npos;
  }

};

TEST_F(JavaGeneratorTest, Basic) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message Foo {
      optional int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto");

  ExpectNoErrors();
}

TEST_F(JavaGeneratorTest, BasicError) {
  CreateTempFile("foo.proto",
                 R"schema(
    syntax = "proto2";
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto:4:7: Expected \"required\", \"optional\", or \"repeated\"");
}

TEST_F(JavaGeneratorTest, ImplicitPresenceLegacyClosedEnumDisallowed) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2023";
    import "third_party/java/protobuf/java_features.proto";
    option features.field_presence = IMPLICIT;
    enum Bar {
      AAA = 0;
    }
    message Foo {
      Bar bar = 1 [features.(pb.java).legacy_closed_enum = true];
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto");

  ExpectErrorSubstring(
      "foo.proto: Field Foo.bar has a closed enum type with implicit "
      "presence.");
}

TEST_F(JavaGeneratorTest, NestInFileClassFeatureDefaultEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      package proto2_unittest;
      option java_generic_services = true;
      message MessageA {
        int32 unused = 1;
        message NestedMessageA {
          int32 unused = 1;
        }

        enum NestedEnumA {
          FOO_DEFAULT = 0;
          FOO_VALUE = 1;
        }
      }
      service MessageB {
        rpc Method(MessageA) returns (MessageA) {}
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectWarningSubstring("Generic services");
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/FooProto.java"));
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/MessageA.java"));
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/MessageB.java"));
  EXPECT_FALSE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/NestedMessageA.java"));
  EXPECT_FALSE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/NestedEnumA.java"));
}

TEST_F(JavaGeneratorTest, NestInFileClassFeatureInNetsedMessageError) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      package proto2_unittest;
      import "third_party/java/protobuf/java_features.proto";
      message Message {
        int32 unused = 1;
        message NestedMessage {
          option features.(pb.java).nest_in_file_class = YES;
          int32 unused = 1;
        }
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring(
      "Feature pb.java.nest_in_file_class only applies to top-level types and "
      "is not allowed to be set on the nested type: "
      "proto2_unittest.Message.NestedMessage");
}

TEST_F(JavaGeneratorTest, NestInFileClassFeatureInNetsedEnumError) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      package proto2_unittest;
      import "third_party/java/protobuf/java_features.proto";
      message Message {
        int32 unused = 1;
        enum NestedEnum {
          option features.(pb.java).nest_in_file_class = YES;
          FOO_DEFAULT = 0;
          FOO_VALUE = 1;
        }
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectErrorSubstring(
      "Feature pb.java.nest_in_file_class only applies to top-level types and "
      "is not allowed to be set on the nested type: "
      "proto2_unittest.Message.NestedEnum");
}

TEST_F(JavaGeneratorTest, SplitNestInFileClassMessageFeatureEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package proto2_unittest;
      message NestedInFileClassMessage {
        option features.(pb.java).nest_in_file_class = YES;
        int32 unused = 1;
      }
      message UnnestedMessage {
        int32 unused = 1;
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/FooProto.java"));
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/UnnestedMessage.java"));
  EXPECT_FALSE(FileGenerated(PACKAGE_PREFIX
                             "proto2_unittest/NestedInFileClassMessage.java"));
}

TEST_F(JavaGeneratorTest, SplitNestInFileClassServiceFeatureEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package proto2_unittest;
      option java_generic_services = true;
      message Dummy {}
      service NestedInFileClassService {
        option features.(pb.java).nest_in_file_class = YES;
        rpc Method(Dummy) returns (Dummy) {}
      }
      service UnnestedService {
        rpc Method(Dummy) returns (Dummy) {}
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectWarningSubstring("Generic services");
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/FooProto.java"));
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/UnnestedService.java"));
  EXPECT_FALSE(FileGenerated(PACKAGE_PREFIX
                             "proto2_unittest/NestedInFileClassService.java"));
}

TEST_F(JavaGeneratorTest, SplitNestInFileClassEnumFeatureEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
      edition = "2024";
      import "third_party/java/protobuf/java_features.proto";
      package proto2_unittest;
      enum NestedInFileClassEnum {
        option features.(pb.java).nest_in_file_class = YES;

        FOO_DEFAULT = 0;
        FOO_VALUE = 1;
      }

      enum UnnestedEnum {
        BAR_DEFAULT = 0;
        BAR_VALUE = 1;
      }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "--experimental_editions foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/FooProto.java"));
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/UnnestedEnum.java"));
  EXPECT_FALSE(FileGenerated(PACKAGE_PREFIX
                             "proto2_unittest/NestedInFileClassEnum.java"));
}

TEST_F(JavaGeneratorTest, LargeClosedEnumDisallowedEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2024";

    import "third_party/java/protobuf/java_features.proto";

    option features.enum_type = CLOSED;

    enum Bar {
      option features.(pb.java).large_enum = true;

      AAA = 0;
      BBB = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto "
      "--experimental_editions");

  ExpectErrorSubstring(
      "foo.proto: Bar is a closed enum and can not be used with the large_enum "
      "feature.  Please migrate to an open enum first, which is a better fit "
      "for extremely large enums.");
}

TEST_F(JavaGeneratorTest, LargeOpenEnumAllowedEdition2024) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2024";

    import "third_party/java/protobuf/java_features.proto";

    enum Bar {
      option features.(pb.java).large_enum = true;

      AAA = 0;
      BBB = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "foo.proto "
      "--experimental_editions");

  ExpectNoErrors();
}

TEST_F(JavaGeneratorTest, LargeEnumUsesLazyInternalValueMapHolder) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2024";
    package proto2_unittest;
    import "third_party/java/protobuf/java_features.proto";

    enum Bar {
      option features.(pb.java).large_enum = true;

      AAA = 0;
      BBB = 1;
    }
  )schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=lite:$tmpdir "
      "foo.proto --experimental_editions");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/Bar.java"));
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX "proto2_unittest/Bar.java",
                                    "return InternalValueMap.INSTANCE;"));
  EXPECT_TRUE(
      FileContainsSubstring(PACKAGE_PREFIX "proto2_unittest/Bar.java",
                            "private static final class InternalValueMap"));
  EXPECT_FALSE(FileContainsSubstring(PACKAGE_PREFIX "proto2_unittest/Bar.java",
                                     "internalValueMap"));
}

TEST_F(JavaGeneratorTest, LargeEnumAliasesEmittedInSameShardAsCanonical) {
  std::string schema = R"schema(
    edition = "2024";
    package proto2_unittest;
    import "third_party/java/protobuf/java_features.proto";
    enum BigAliasedEnum {
      option allow_alias = true;
      option features.(pb.java).large_enum = true;
  )schema";
  for (int i = 0; i <= 1996; ++i) {
    absl::StrAppend(&schema, "  VAL", i, " = ", i, ";\n");
  }
  absl::StrAppend(
      &schema,
      "  VAL1_ALIAS = 1;\n  VAL998_ALIAS = 998;\n  VAL1995_ALIAS = 1995;\n}\n");
  CreateTempFile("big_aliased.proto", schema);

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "big_aliased.proto --experimental_editions");

  ExpectNoErrors();
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/BigAliasedEnum.java"));
  // VAL1_ALIAS, VAL998_ALIAS, and VAL1995_ALIAS must be emitted in the same
  // shard as their canonical values using direct same-interface references
  // rather than cross-interface references:
  // - BigAliasedEnum0 has 999 constants (VAL0..VAL997 plus VAL1_ALIAS), because
  //   (VAL998, VAL998_ALIAS) has group_size = 2 (999 + 2 > 1000) and is pushed
  //   to the start of BigAliasedEnum1.
  // - BigAliasedEnum1 has 1000 constants (VAL998..VAL1995 plus VAL998_ALIAS and
  //   VAL1995_ALIAS), because (VAL1995, VAL1995_ALIAS) starts at size 998
  //   (998 + 2 == 1000 <= 1000) and fits at the end of BigAliasedEnum1, while
  //   VAL1996 starts BigAliasedEnum2.
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "proto2_unittest/BigAliasedEnum.java",
      "public static final BigAliasedEnum VAL1_ALIAS = VAL1;"));
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX
                                    "proto2_unittest/BigAliasedEnum.java",
                                    R"(VAL996, VAL997
    };
  }
}
interface BigAliasedEnum1 {

  /**
   * <code>VAL998 = 998;</code>
   */
  public static final BigAliasedEnum VAL998 = new BigAliasedEnum(998, 998, "VAL998");
  public static final int VAL998_VALUE = 998;
  /**
   * <code>VAL998_ALIAS = 998;</code>
   */
  public static final BigAliasedEnum VAL998_ALIAS = VAL998;
  public static final int VAL998_ALIAS_VALUE = 998;)"));
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX
                                    "proto2_unittest/BigAliasedEnum.java",
                                    R"(VAL1994, VAL1995
    };
  }
}
interface BigAliasedEnum2 {

  /**
   * <code>VAL1996 = 1996;</code>
   */
  public static final BigAliasedEnum VAL1996 = new BigAliasedEnum(1996, 1996, "VAL1996");)"));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "proto2_unittest/BigAliasedEnum.java",
      "public static final BigAliasedEnum VAL1995_ALIAS = VAL1995;"));
  EXPECT_FALSE(FileContainsSubstring(PACKAGE_PREFIX
                                     "proto2_unittest/BigAliasedEnum.java",
                                     "interface BigAliasedEnum3"));
  EXPECT_FALSE(FileContainsSubstring(PACKAGE_PREFIX
                                     "proto2_unittest/BigAliasedEnum.java",
                                     "BigAliasedEnum0.VAL1"));
  EXPECT_FALSE(FileContainsSubstring(PACKAGE_PREFIX
                                     "proto2_unittest/BigAliasedEnum.java",
                                     "BigAliasedEnum0.VAL998"));
  EXPECT_FALSE(FileContainsSubstring(PACKAGE_PREFIX
                                     "proto2_unittest/BigAliasedEnum.java",
                                     "BigAliasedEnum1.VAL998"));
}
struct OpenEnumUnrecognizedTestCase {
  absl::string_view test_name;
  bool lite;
  absl::string_view enum_options;
  absl::string_view enum_values;
  absl::string_view expected_unrecognized;
};

class OpenEnumUnrecognizedTest
    : public JavaGeneratorTest,
      public testing::WithParamInterface<OpenEnumUnrecognizedTestCase> {};

TEST_P(OpenEnumUnrecognizedTest, AvoidsNegativeValueCollision) {
  const OpenEnumUnrecognizedTestCase& test_case = GetParam();
  absl::string_view imports =
      absl::StrContains(test_case.enum_options, "pb.java")
          ? "import \"third_party/java/protobuf/java_features.proto\";"
          : "";
  CreateTempFile("foo.proto",
                 absl::Substitute(
                     R"schema(
    edition = "2024";
    package proto2_unittest;

    $0

    enum TestEnum {
      $1
      $2
    }
  )schema",
                     imports, test_case.enum_options, test_case.enum_values));

  RunProtoc(absl::StrCat("protocol_compiler --proto_path=$tmpdir --java_out=",
                         test_case.lite ? "lite:" : "",
                         "$tmpdir foo.proto --experimental_editions"));

  ExpectNoErrors();
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX
                                    "proto2_unittest/TestEnum.java",
                                    test_case.expected_unrecognized));
}

INSTANTIATE_TEST_SUITE_P(
    OpenEnumUnrecognizedTests, OpenEnumUnrecognizedTest,
    testing::Values(
        OpenEnumUnrecognizedTestCase{
            /*test_name=*/"StandardFull",
            /*lite=*/false,
            /*enum_options=*/"",
            /*enum_values=*/"ZERO = 0;\nNEG_ONE = -1;",
            /*expected_unrecognized=*/"UNRECOGNIZED(-2),",
        },
        OpenEnumUnrecognizedTestCase{
            /*test_name=*/"StandardLite",
            /*lite=*/true,
            /*enum_options=*/"",
            /*enum_values=*/"ZERO = 0;\nNEG_ONE = -1;",
            /*expected_unrecognized=*/"UNRECOGNIZED(-2),",
        },
        OpenEnumUnrecognizedTestCase{
            /*test_name=*/"AliasingFull",
            /*lite=*/false,
            /*enum_options=*/"option allow_alias = true;",
            /*enum_values=*/
            "ZERO = 0;\nZERO_ALIAS = 0;\nONE = 1;\nNEG_ONE = -1;",
            /*expected_unrecognized=*/"UNRECOGNIZED(-1, -2),",
        },
        OpenEnumUnrecognizedTestCase{
            /*test_name=*/"AliasingLite",
            /*lite=*/true,
            /*enum_options=*/"option allow_alias = true;",
            /*enum_values=*/
            "ZERO = 0;\nZERO_ALIAS = 0;\nONE = 1;\nNEG_ONE = -1;",
            /*expected_unrecognized=*/"UNRECOGNIZED(-2),",
        },
        OpenEnumUnrecognizedTestCase{
            /*test_name=*/"LargeFull",
            /*lite=*/false,
            /*enum_options=*/"option features.(pb.java).large_enum = true;",
            /*enum_values=*/"ZERO = 0;\nNEG_ONE = -1;\nNEG_TWO = -2;",
            /*expected_unrecognized=*/
            "UNRECOGNIZED = new TestEnum(-3, 3, \"UNRECOGNIZED\");",
        },
        OpenEnumUnrecognizedTestCase{
            /*test_name=*/"LargeLite",
            /*lite=*/true,
            /*enum_options=*/"option features.(pb.java).large_enum = true;",
            /*enum_values=*/"ZERO = 0;\nNEG_ONE = -1;\nNEG_TWO = -2;",
            /*expected_unrecognized=*/
            "UNRECOGNIZED = new TestEnum(-3, \"UNRECOGNIZED\");",
        }),
    [](const testing::TestParamInfo<OpenEnumUnrecognizedTestCase>& info) {
      return std::string(info.param.test_name);
    });
TEST_F(JavaGeneratorTest, LargeEnumDisallowedEdition2023) {
  CreateTempFile("foo.proto",
                 R"schema(
edition = "2023";

import "third_party/java/protobuf/java_features.proto";

enum Bar {
option features.(pb.java).large_enum = true;

AAA = 0;
BBB = 1;
}
)schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto "
      "--experimental_editions");

  ExpectErrorSubstring(
      "foo.proto:6:6: pb.JavaFeatures.large_enum wasn't introduced "
      "until edition 2024 and can't be used in edition 2023");
}

TEST_F(JavaGeneratorTest,
       InvalidConflictingProtoSuffixedMessageNameEdition2024) {
  CreateTempFile("test_file_name.proto",
                 R"schema(
      edition = "2024";
      package foo;
      message TestFileNameProto {
        int32 field = 1;
      }
      )schema");

  RunProtoc(
      "protocol_compiler --experimental_editions --java_out=$tmpdir "
      "-I$tmpdir test_file_name.proto");

  ExpectErrorSubstring(
      "Cannot generate Java output because the file's outer "
      "class name, \"TestFileNameProto\", matches the name "
      "of one of the types declared inside it");
}

TEST_F(JavaGeneratorTest, InvalidConflictingNestedEnumName) {
  CreateTempFile("test_file_name.proto",
                 R"schema(
      edition = "2024";
      package foo;
      message TestNameConflict {
        enum TestNameConflict {
          UNSPECIFIED = 0;
        }
      }
      )schema");

  RunProtoc(
      "protocol_compiler --experimental_editions --java_out=$tmpdir "
      "-I$tmpdir test_file_name.proto");

  ExpectErrorSubstring(
      "would be an enum nested inside a class with the same name");
}

TEST_F(JavaGeneratorTest, ExtensionsOptionImportsAreUnknown) {
  CreateTempFile("custom_option.proto", R"schema(
      edition = "2024";
      package foo;
      import "google/protobuf/descriptor.proto";
      extend google.protobuf.FileOptions {
        int32 option_import_file_opt = 9998;
      }
      extend google.protobuf.MessageOptions {
        int32 option_import_message_opt = 9998;
      }
      extend google.protobuf.FieldOptions {
        int32 option_import_field_opt = 9998;
      }
      )schema");
  CreateTempFile("foo.proto", R"schema(
      edition = "2024";
      package foo;
      import "google/protobuf/descriptor.proto";
      import option "custom_option.proto";

      // Should be an unknown custom option for the Java generator if not polluted by pool.
      option (option_import_file_opt) = 123;
      option (file_opt) = 123;

      message Foo {
        option (option_import_message_opt) = 456;
        option (message_opt) = 456;
        int32 bar = 1 [(option_import_field_opt) = 789, (field_opt) = 789];
      }
      extend google.protobuf.FileOptions {
        int32 file_opt = 9999;
      }
      extend google.protobuf.MessageOptions {
        int32 message_opt = 9999;
      }
      extend google.protobuf.FieldOptions {
        int32 field_opt = 9999;
      }
      )schema");

  RunProtoc(
      "protocol_compiler --java_out=$tmpdir --experimental_editions "
      "-I$tmpdir foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "foo/FooProto.java"));

  EXPECT_FALSE(
      FileContainsSubstring(PACKAGE_PREFIX "foo/FooProto.java",
                            "foo.CustomOptionProto.optionImportFileOpt"));
  EXPECT_FALSE(
      FileContainsSubstring(PACKAGE_PREFIX "foo/FooProto.java",
                            "foo.CustomOptionProto.optionImportMessageOpt"));
  EXPECT_FALSE(
      FileContainsSubstring(PACKAGE_PREFIX "foo/FooProto.java",
                            "foo.CustomOptionProto.optionImportFieldOpt"));

  EXPECT_TRUE(
      FileContainsSubstring(PACKAGE_PREFIX "foo/FooProto.java",
                            absl::StrCat("registry.add(", PACKAGE_IMPORT_PREFIX,
                                         "foo.FooProto.fileOpt);")));
  EXPECT_TRUE(
      FileContainsSubstring(PACKAGE_PREFIX "foo/FooProto.java",
                            absl::StrCat("registry.add(", PACKAGE_IMPORT_PREFIX,
                                         "foo.FooProto.messageOpt);")));
  EXPECT_TRUE(
      FileContainsSubstring(PACKAGE_PREFIX "foo/FooProto.java",
                            absl::StrCat("registry.add(", PACKAGE_IMPORT_PREFIX,
                                         "foo.FooProto.fieldOpt);")));
}


TEST_F(JavaGeneratorTest, ExtensionsPublicImportsAreKnown) {
  CreateTempFile("custom_option.proto", R"schema(
      edition = "2024";
      package foo;
      import "google/protobuf/descriptor.proto";
      extend google.protobuf.FileOptions {
        int32 transitive_public_import_file_opt = 9997;
      }
      )schema");
  CreateTempFile("import_public2.proto", R"schema(
      edition = "2024";
      package foo;
      import "google/protobuf/descriptor.proto";
      import public "custom_option.proto";
      extend google.protobuf.FileOptions {
        int32 public_import_file_opt = 9998;
      }
      )schema");
  CreateTempFile("import_public1.proto", R"schema(
      edition = "2024";
      package foo;
      import public "import_public1.proto";
      message MyMessage {}
      )schema");
  CreateTempFile("foo.proto", R"schema(
      edition = "2024";
      package foo;
      import "google/protobuf/descriptor.proto";
      import public "import_public2.proto";

      option (transitive_public_import_file_opt) = 123;
      option (public_import_file_opt) = 123;
      option (file_opt) = 123;

      extend google.protobuf.FileOptions {
        int32 file_opt = 9999;
      }
      )schema");
  RunProtoc(
      "protocol_compiler --java_out=$tmpdir --experimental_editions -I$tmpdir "
      "foo.proto");
  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "foo/FooProto.java"));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "foo/FooProto.java",
      absl::StrCat("registry.add(", PACKAGE_IMPORT_PREFIX,
                   "foo.ImportPublic2Proto.publicImportFileOpt);")));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "foo/FooProto.java",
      absl::StrCat("registry.add(", PACKAGE_IMPORT_PREFIX,
                   "foo.CustomOptionProto.transitivePublicImportFileOpt);")));
  EXPECT_TRUE(
      FileContainsSubstring(PACKAGE_PREFIX "foo/FooProto.java",
                            absl::StrCat("registry.add(", PACKAGE_IMPORT_PREFIX,
                                         "foo.FooProto.fileOpt);")));
}


TEST_F(JavaGeneratorTest, BitFieldsAreTransient) {
  CreateTempFile("foo.proto",
                 R"schema(
    edition = "2024";
    package proto2_unittest;
    message Foo {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir foo.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX "proto2_unittest/Foo.java"));
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX "proto2_unittest/Foo.java",
                                    "private transient int bitField0_;"));
}

TEST_F(JavaGeneratorTest, MultipleBitFieldsAreTransient) {
  std::string schema = R"schema(
    edition = "2024";
    package proto2_unittest;
    message MultiBitField {
  )schema";
  for (int i = 1; i <= 33; ++i) {
    absl::StrAppend(&schema, "  int32 f", i, " = ", i, ";\n");
  }
  absl::StrAppend(&schema, "}\n");
  CreateTempFile("multi.proto", schema);

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir multi.proto");

  ExpectNoErrors();
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "proto2_unittest/MultiBitField.java"));
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX
                                    "proto2_unittest/MultiBitField.java",
                                    "private transient int bitField0_;"));
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX
                                    "proto2_unittest/MultiBitField.java",
                                    "private transient int bitField1_;"));
}

TEST_F(JavaGeneratorTest, AllowlistedBitFieldsAreNotTransient) {
  CreateTempFile("allowlisted.proto",
                 R"schema(
    edition = "2024";
    package com.google.gson.protobuf;
    message TestAllTypes {
      int32 bar = 1;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "allowlisted.proto");

  ExpectNoErrors();
  EXPECT_TRUE(FileGenerated(PACKAGE_PREFIX
                            "com/google/gson/protobuf/TestAllTypes.java"));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/gson/protobuf/TestAllTypes.java",
      "private int bitField0_;"));
  EXPECT_FALSE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/gson/protobuf/TestAllTypes.java",
      "private transient int bitField0_;"));
}

TEST_F(JavaGeneratorTest, OneofHasBitsBuildPartial) {
  CreateTempFile("oneof_has_bits.proto",
                 R"schema(
    syntax = "proto2";
    package com.google.protos;
    option java_multiple_files = true;
    message SubMsg {
      optional int32 x = 1;
    }
    message TestOneofHasBits {
      optional int32 regular_int = 1;
      oneof my_oneof {
        int32 oneof_int = 2;
        string oneof_str = 3;
        SubMsg oneof_msg = 4;
      }
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_out=$tmpdir "
      "oneof_has_bits.proto");

  ExpectNoErrors();
  EXPECT_TRUE(
      FileGenerated(PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java"));
  // Verify message typed field in oneof is treated like regular message typed
  // fields (checked via from_bitField0_ & 0x00000008).
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX
                                    "com/google/protos/TestOneofHasBits.java",
                                    "((from_bitField0_ & 0x00000008) != 0)"));
  // Verify non-message oneof fields are grouped together by bitField
  // membership (0x00000002 | 0x00000004 = 0x00000006).
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java",
      "to_bitField0_ |= (from_bitField0_ & 0x00000006);"));
  // Verify clear<FieldName>() clears the individual has-bit.
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java",
      "public Builder clearOneofInt() {\n"
      "      if (myOneofCase_ == 2) {\n"
      "        myOneofCase_ = 0;\n"
      "        bitField0_ = (bitField0_ & ~0x00000002);\n"
      "        myOneof_ = null;\n"
      "        onChanged();\n"
      "      }\n"
      "      return this;\n"
      "    }"));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java",
      "public Builder clearOneofStr() {\n"
      "      if (myOneofCase_ == 3) {\n"
      "        myOneofCase_ = 0;\n"
      "        bitField0_ = (bitField0_ & ~0x00000004);\n"
      "        myOneof_ = null;\n"
      "        onChanged();\n"
      "      }\n"
      "      return this;\n"
      "    }"));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java",
      "public Builder clearOneofMsg() {\n"
      "      if (oneofMsgBuilder_ == null) {\n"
      "        if (myOneofCase_ == 4) {\n"
      "          myOneofCase_ = 0;\n"
      "          bitField0_ = (bitField0_ & ~0x00000008);\n"
      "          myOneof_ = null;\n"
      "          onChanged();\n"
      "        }\n"
      "      } else {\n"
      "        if (myOneofCase_ == 4) {\n"
      "          myOneofCase_ = 0;\n"
      "          bitField0_ = (bitField0_ & ~0x00000008);\n"
      "          myOneof_ = null;\n"
      "        }\n"
      "        oneofMsgBuilder_.clear();\n"
      "      }\n"
      "      return this;\n"
      "    }"));
  // Verify parse<FieldName>() methods encapsulate oneof parsing and
  // bookkeeping.
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java",
      "private void parseOneofInt(\n"
      "        com.google.protobuf.CodedInputStream input)\n"
      "        throws java.io.IOException {\n"
      "      myOneof_ = input.readInt32();\n"
      "      switch (myOneofCase_) {\n"
      "      default:\n"
      "        clearMyOneofHasBits(); // fallthrough\n"
      "      case 0:\n"
      "        myOneofCase_ = 2;\n"
      "        bitField0_ |= 0x00000002; // fallthrough\n"
      "      case 2:\n"
      "        break;\n"
      "      }\n"
      "    }"));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java",
      "private void parseOneofStr(\n"
      "        com.google.protobuf.CodedInputStream input)\n"
      "        throws java.io.IOException {"));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java",
      "private void parseOneofMsg(\n"
      "        com.google.protobuf.CodedInputStream input,\n"
      "        com.google.protobuf.ExtensionRegistryLite extensionRegistry)\n"
      "        throws java.io.IOException {"));
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX
                                    "com/google/protos/TestOneofHasBits.java",
                                    "parseOneofInt(input);"));
  EXPECT_TRUE(FileContainsSubstring(PACKAGE_PREFIX
                                    "com/google/protos/TestOneofHasBits.java",
                                    "parseOneofStr(input);"));
  EXPECT_TRUE(FileContainsSubstring(
      PACKAGE_PREFIX "com/google/protos/TestOneofHasBits.java",
      "parseOneofMsg(input, extensionRegistry);"));
}

TEST_F(JavaGeneratorTest, LiteExtensionUtf8ValidationEditions) {
  CreateTempFile("editions_ext.proto",
                 R"schema(
    edition = "2023";
    package proto2_unittest;
    import "third_party/java/protobuf/java_features.proto";
    option java_outer_classname = "EditionsExtProto";

    message Extendable {
      extensions 100 to 200;
    }

    message Scope {
      extend Extendable {
        string nested_ext_verify = 110;
        string nested_ext_none = 111 [features.utf8_validation = NONE];
      }
    }

    extend Extendable {
      string ext_verify_default = 101;
      repeated string rep_ext_verify_default = 102;
      string ext_java_verify = 103 [
        features.utf8_validation = NONE,
        features.(pb.java).utf8_validation = VERIFY
      ];
      repeated string rep_ext_java_verify = 104 [
        features.utf8_validation = NONE,
        features.(pb.java).utf8_validation = VERIFY
      ];
      string ext_none = 105 [features.utf8_validation = NONE];
      repeated string rep_ext_none = 106 [features.utf8_validation = NONE];
      bytes ext_bytes = 107;
      int32 ext_int32 = 108;
      repeated int32 rep_ext_int32 = 109;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_opt=lite "
      "--java_out=$tmpdir editions_ext.proto");

  ExpectNoErrors();
  const std::string output_file =
      PACKAGE_PREFIX "proto2_unittest/EditionsExtProto.java";
  EXPECT_TRUE(FileGenerated(output_file));

  // Editions VERIFY singular and repeated string extensions emit 8-arg calls
  // with checkUtf8 = true.
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "extVerifyDefault = com.google.protobuf.GeneratedMessageLite\n"
      "          .newSingularGeneratedExtension(\n"
      "        " PACKAGE_IMPORT_PREFIX
      "proto2_unittest.EditionsExtProto.Extendable.getDefaultInstance(),\n"
      "        \"\",\n"
      "        null,\n"
      "        null,\n"
      "        101,\n"
      "        com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "        true,\n"
      "        java.lang.String.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "repExtVerifyDefault = com.google.protobuf.GeneratedMessageLite\n"
      "          .newRepeatedGeneratedExtension(\n"
      "        " PACKAGE_IMPORT_PREFIX
      "proto2_unittest.EditionsExtProto.Extendable.getDefaultInstance(),\n"
      "        null,\n"
      "        null,\n"
      "        102,\n"
      "        com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "        false,\n"
      "        true,\n"
      "        java.lang.String.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        103,\n"
      "        com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "        true,\n"
      "        java.lang.String.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        104,\n"
      "        com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "        false,\n"
      "        true,\n"
      "        java.lang.String.class);"));
  // Nested message-scoped extension also honors VERIFY vs NONE.
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "          110,\n"
      "          com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "          true,\n"
      "          java.lang.String.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "          111,\n"
      "          com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "          java.lang.String.class);"));

  // Editions NONE and non-string extensions emit 7-arg calls without checkUtf8.
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        105,\n"
      "        com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "        java.lang.String.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        106,\n"
      "        com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "        false,\n"
      "        java.lang.String.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        107,\n"
      "        com.google.protobuf.WireFormat.FieldType.BYTES,\n"
      "        com.google.protobuf.ByteString.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        108,\n"
      "        com.google.protobuf.WireFormat.FieldType.INT32,\n"
      "        java.lang.Integer.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        109,\n"
      "        com.google.protobuf.WireFormat.FieldType.INT32,\n"
      "        true,\n"
      "        java.lang.Integer.class);"));
}

TEST_F(JavaGeneratorTest, LiteExtensionUtf8ValidationLegacyProto2Unchanged) {
  CreateTempFile("proto2_ext.proto",
                 R"schema(
    syntax = "proto2";
    package proto2_unittest;
    option java_outer_classname = "Proto2ExtProto";
    option java_string_check_utf8 = true;

    message Extendable {
      extensions 100 to 200;
    }

    extend Extendable {
      optional string ext_checked = 101;
      repeated string rep_ext_checked = 102;
    })schema");

  RunProtoc(
      "protocol_compiler --proto_path=$tmpdir --java_opt=lite "
      "--java_out=$tmpdir proto2_ext.proto");

  ExpectNoErrors();
  const std::string output_file =
      PACKAGE_PREFIX "proto2_unittest/Proto2ExtProto.java";
  EXPECT_TRUE(FileGenerated(output_file));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        101,\n"
      "        com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "        java.lang.String.class);"));
  EXPECT_TRUE(FileContainsSubstring(
      output_file,
      "        102,\n"
      "        com.google.protobuf.WireFormat.FieldType.STRING,\n"
      "        false,\n"
      "        java.lang.String.class);"));
}

}  // namespace
}  // namespace java
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
