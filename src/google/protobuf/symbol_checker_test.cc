// Protocol Buffers - Google's data interchange format
// Copyright 2025 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/symbol_checker.h"

#include <utility>

#include "google/protobuf/descriptor.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor_test_utils.h"
#include "google/protobuf/feature_resolver.h"

// Must be included last.
#include "google/protobuf/port_def.inc"

using ::testing::NotNull;

namespace {
absl::Status GetStatus(const absl::Status& s) { return s; }
template <typename T>
absl::Status GetStatus(const absl::StatusOr<T>& s) {
  return s.status();
}
MATCHER_P2(StatusIs, status, message,
           absl::StrCat(".status() is ", testing::PrintToString(status))) {
  return GetStatus(arg).code() == status &&
         testing::ExplainMatchResult(message, GetStatus(arg).message(),
                                     result_listener);
}
#define EXPECT_OK(x) EXPECT_THAT(x, StatusIs(absl::StatusCode::kOk, testing::_))
#define ASSERT_OK(x) ASSERT_THAT(x, StatusIs(absl::StatusCode::kOk, testing::_))
}  // namespace

namespace google {
namespace protobuf {
namespace descriptor_unittest {

class SymbolCheckerTest : public ValidationErrorTest {
 protected:
  void SetUp() override {
    ValidationErrorTest::SetUp();

    auto default_spec = FeatureResolver::CompileDefaults(
        FeatureSet::descriptor(), {}, EDITION_PROTO2, EDITION_99999_TEST_ONLY);
    ASSERT_OK(default_spec);
    ASSERT_OK(pool_.SetFeatureSetDefaults(std::move(default_spec).value()));
  }
};

TEST_F(SymbolCheckerTest, VisibilityFromSame) {
  pool_.EnforceSymbolVisibility(true);
  EXPECT_THAT(ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        local message LocalMessage {
        }
        export message ExportMessage {
          LocalMessage foo = 1;
        }
        )schema"),
              NotNull());
}

TEST_F(SymbolCheckerTest, ExplicitVisibilityFromOther) {
  pool_.EnforceSymbolVisibility(true);
  ASSERT_THAT(ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        local message LocalMessage {
        }
        export message ExportMessage {
        }
        )schema"),
              NotNull());

  ParseAndBuildFileWithErrorSubstr(
      "importer.proto",
      R"schema(
        edition = "2024";
        import "vis.proto";

        message BadImport {
          vis.test.LocalMessage foo = 1;
        }
      )schema",
      "importer.proto: BadImport.foo: TYPE: Symbol \"vis.test.LocalMessage\", "
      "defined in \"vis.proto\"  is not visible from \"importer.proto\". It is "
      "explicitly marked 'local' and cannot be accessed outside its own "
      "file\n");
}

TEST_F(SymbolCheckerTest, ExplicitVisibilityFromOtherDisabled) {
  pool_.EnforceSymbolVisibility(false);
  ASSERT_THAT(ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        local message LocalMessage {
        }
        export message ExportMessage {
        }
        )schema"),
              NotNull());

  EXPECT_THAT(ParseAndBuildFile("importer.proto",
                                R"schema(
        edition = "2024";
        import "vis.proto";

        message BadImport {
          vis.test.LocalMessage foo = 1;
        }
      )schema"),
              NotNull());
}

TEST_F(SymbolCheckerTest, Edition2024DefaultVisibilityFromOther) {
  pool_.EnforceSymbolVisibility(true);
  ASSERT_THAT(ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        message TopLevelMessage {
          message NestedMessage {
          }
        }
        )schema"),
              NotNull());

  ASSERT_THAT(ParseAndBuildFile("good_importer.proto", R"schema(
        edition = "2024";
        import "vis.proto";

        message GoodImport {
          vis.test.TopLevelMessage foo = 1;
        }
        )schema"),
              NotNull());

  ParseAndBuildFileWithErrorSubstr(
      "bad_importer.proto", R"schema(
        edition = "2024";
        import "vis.proto";

        message BadImport {
          vis.test.TopLevelMessage.NestedMessage foo = 1;
        }
        )schema",

      "bad_importer.proto: BadImport.foo: TYPE: Symbol "
      "\"vis.test.TopLevelMessage.NestedMessage\", defined in \"vis.proto\"  "
      "is not visible from \"bad_importer.proto\". It defaulted to local from "
      "file-level 'option features.default_symbol_visibility = "
      "'EXPORT_TOP_LEVEL'; and cannot be accessed outside its own file\n");
}

TEST_F(SymbolCheckerTest, VisibilityFromLocalExtender) {
  pool_.EnforceSymbolVisibility(true);
  ASSERT_THAT(ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        local message LocalExtendee {
          extensions 1 to 100;
        }
        )schema"),
              NotNull());

  ParseAndBuildFileWithErrorSubstr(
      "bad_importer.proto", R"schema(
        edition = "2024";
        import "vis.proto";

        extend vis.test.LocalExtendee {
          string bar = 1;
        }
      )schema",
      "bad_importer.proto: bar: EXTENDEE: Symbol \"vis.test.LocalExtendee\", "
      "defined in \"vis.proto\" target of extend is not visible from "
      "\"bad_importer.proto\". It is explicitly marked 'local' and cannot be "
      "accessed outside its own file\n");
}

TEST_F(SymbolCheckerTest, IsEnumNamespaceMessage) {
  pool_.EnforceSymbolVisibility(true);
  const FileDescriptor* file = ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        option features.default_symbol_visibility = STRICT;

        local message EnumNamespaceMessage {
          export enum Enum {
            FOO = 0;
          }
          reserved 1 to max;
        }

        enum TopLevelEnum {
          BAR = 0;
        }
        )schema");

  ASSERT_THAT(file, NotNull());

  const Descriptor* namespace_message =
      file->FindMessageTypeByName("EnumNamespaceMessage");
  ASSERT_THAT(namespace_message, NotNull());

  ASSERT_TRUE(SymbolChecker::IsEnumNamespaceMessage(*namespace_message));

  const EnumDescriptor* enum_descriptor =
      namespace_message->FindEnumTypeByName("Enum");
  ASSERT_THAT(enum_descriptor, NotNull());

  ASSERT_TRUE(SymbolChecker::IsNamespacedEnum(*enum_descriptor));

  // Make sure it can be imported.
  ASSERT_THAT(ParseAndBuildFile("other.proto", R"schema(
        edition = "2024";
        package vis.test;

        import "vis.proto";

        message OtherMessage {
          vis.test.EnumNamespaceMessage.Enum foo = 1;
        }
        )schema"),
              NotNull());

  const EnumDescriptor* top_enum_descriptor =
      file->FindEnumTypeByName("TopLevelEnum");
  ASSERT_THAT(top_enum_descriptor, NotNull());
  ASSERT_FALSE(SymbolChecker::IsNamespacedEnum(*top_enum_descriptor));
}

TEST_F(SymbolCheckerTest, IsNamespacedEnumLocalDefault) {
  pool_.EnforceSymbolVisibility(true);
  const FileDescriptor* file = ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        option features.default_symbol_visibility = LOCAL_ALL;
        message EnumNamespaceMessage {
          export enum Enum {
            FOO = 0;
          }
          reserved 1 to max;
        }

        enum TopLevelEnum {
          BAR = 0;
        }
        )schema");

  ASSERT_THAT(file, NotNull());

  const Descriptor* namespace_message =
      file->FindMessageTypeByName("EnumNamespaceMessage");
  ASSERT_THAT(namespace_message, NotNull());

  const EnumDescriptor* enum_descriptor =
      namespace_message->FindEnumTypeByName("Enum");
  ASSERT_THAT(enum_descriptor, NotNull());

  ASSERT_TRUE(SymbolChecker::IsNamespacedEnum(*enum_descriptor));
}

TEST_F(SymbolCheckerTest, IsNamespacedEnumExplicitlyLocalParent) {
  pool_.EnforceSymbolVisibility(true);
  const FileDescriptor* file = ParseAndBuildFile("vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        local message EnumNamespaceMessage {
          enum Enum {
            FOO = 0;
          }
          reserved 1 to max;
        }

        enum TopLevelEnum {
          BAR = 0;
        }
        )schema");

  ASSERT_THAT(file, NotNull());

  const Descriptor* namespace_message =
      file->FindMessageTypeByName("EnumNamespaceMessage");
  ASSERT_THAT(namespace_message, NotNull());

  const EnumDescriptor* enum_descriptor =
      namespace_message->FindEnumTypeByName("Enum");
  ASSERT_THAT(enum_descriptor, NotNull());

  ASSERT_TRUE(SymbolChecker::IsNamespacedEnum(*enum_descriptor));
}

TEST_F(SymbolCheckerTest, IsNotEnumNamespaceMessage) {
  pool_.EnforceSymbolVisibility(true);

  // message is not local
  ParseAndBuildFileWithErrorSubstr(
      "vis.proto", R"schema(
        edition = "2024";
        package vis.test;

        option features.default_symbol_visibility = STRICT;

        export message EnumNamespaceMessage {
          export enum Enum {
            FOO = 0;
          }
          reserved 1 to max;
        }
        )schema",
      "is a nested enum and cannot be marked `export` with STRICT");

  // message doesn't have reserved range.
  ParseAndBuildFileWithErrorSubstr(
      "vis2.proto", R"schema(
        edition = "2024";
        package vis.test;

        option features.default_symbol_visibility = STRICT;

        local message EnumNamespaceMessage {
          export enum Enum {
            FOO = 0;
          }
        }
        )schema",
      "is a nested enum and cannot be marked `export` with STRICT");

  // message doesn't have full reserved range.
  ParseAndBuildFileWithErrorSubstr(
      "vis2.proto", R"schema(
        edition = "2024";
        package vis.test;

        option features.default_symbol_visibility = STRICT;

        local message EnumNamespaceMessage {
          export enum Enum {
            FOO = 0;
          }
          reserved 1 to 10;
        }
        )schema",
      "is a nested enum and cannot be marked `export` with STRICT");
}

}  // namespace descriptor_unittest
}  // namespace protobuf
}  // namespace google

#include "google/protobuf/port_undef.inc"
