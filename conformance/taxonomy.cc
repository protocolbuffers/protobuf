// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "conformance/taxonomy.h"

#include <cstddef>
#include <string>

#include "absl/base/no_destructor.h"
#include "absl/base/nullability.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::conformance::ConformanceRequest;

struct SuiteSection {
  absl::string_view suite;
  ConformanceSection section;
};

// The section of every conformance gtest suite.  A suite lands in the section
// its legacy counterpart (the ConformanceTestSuite method it replaced) had in
// the original taxonomy; suites without a one-to-one counterpart are filed by
// topic.
constexpr SuiteSection kSuiteSections[] = {
    // Binary suites (conformance_suite "binary").
    {"DelimitedExtensionTest", ConformanceSection::kEditions_Delimited_Fields},
    {"DelimitedFieldTest", ConformanceSection::kEditions_Delimited_Fields},
    {"IllegalTagsTest", ConformanceSection::kWire_GroupsUnknown_WireTypes},
    {"UnknownWireTypeTest", ConformanceSection::kWire_GroupsUnknown_WireTypes},
    {"UnmatchedGroupTest", ConformanceSection::kWire_GroupsUnknown_WireTypes},
    // The legacy merge tests ran inside TestValidDataForRepeatedScalarMessage;
    // the map and oneof merge tests are refiled by kTestSections below.
    // TODO: b/564551336 - confirm section
    {"MergeTest", ConformanceSection::kWire_LengthDelimited_Submessages},
    {"MessageSetTest", ConformanceSection::kWire_MessageSets},
    {"ValidDataOneofTest", ConformanceSection::kSemantics_Oneofs_Types},
    // The legacy TestOneofMessage().
    {"OneofZeroTest", ConformanceSection::kSemantics_Oneofs_Unknown},
    {"PrematureEofTest", ConformanceSection::kWire_Varint_Eof},
    {"PrematureEofInDelimitedDataTest", ConformanceSection::kWire_Varint_Eof},
    {"PrematureEofInPackedFieldTest", ConformanceSection::kWire_Varint_Eof},
    {"PrematureEofInSubmessageValueTest", ConformanceSection::kWire_Varint_Eof},
    {"UnknownFieldsTest",
     ConformanceSection::kWire_GroupsUnknown_UnknownFields},
    {"UnstableEditionTest", ConformanceSection::kEditions_Features_Unstable},
    // The legacy TestInvalidUtf8String() and RunUtf8ValidationTests().
    {"Utf8StringTest", ConformanceSection::kWire_LengthDelimited_Utf8},
    {"Utf8ExtensionTest", ConformanceSection::kEditions_Utf8_Validation},
    {"MapEntryWireTypeMismatchTest", ConformanceSection::kSemantics_Maps_Types},
    {"ValidDataMapTest", ConformanceSection::kSemantics_Maps_Types},
    // The legacy suite filed the valid-data tests by the field's wire encoding
    // (varint integers, fixed, bool/zigzag, ...); these suites are
    // parameterized by the field type, and SectionOf() refines their section
    // from it (see kFieldTypeSections).  The suite entry only covers a test
    // whose parameters carry no field type.
    {"ValidDataScalarTest", ConformanceSection::kWire_Varint_Integers},
    {"RepeatedScalarSelectsLastTest",
     ConformanceSection::kWire_Varint_Integers},
    {"ValidDataRepeatedTest", ConformanceSection::kWire_Varint_Integers},
    {"ValidDataRepeatedNonPackableTest",
     ConformanceSection::kWire_LengthDelimited_Primitives},

    // Performance suites (conformance_suite "performance").  The recursion
    // limit suites check required behavior (the legacy EnforceDepthLimit
    // tests of RunRecursionLimitTests(), which the original taxonomy filed
    // under wire.length_delimited); the rest are benchmarks.
    {"Edition2023RecursionLimitPerformanceTest",
     ConformanceSection::kWire_LengthDelimited_RecursionLimit},
    {"RecursionLimitPerformanceTest",
     ConformanceSection::kWire_LengthDelimited_RecursionLimit},
    {"MergeMessagePerformanceTest",
     ConformanceSection::kInformational_Performance_Benchmarks},
    {"UnknownFieldsPerformanceTest",
     ConformanceSection::kInformational_Performance_Benchmarks},
    // TODO: b/564551336 - the legacy text performance tests were filed under
    // text_format.performance (kTextFormat_Performance_Merge); confirm whether
    // they are a benchmark or a required merge test.
    {"TextPerformanceTest",
     ConformanceSection::kInformational_Performance_Benchmarks},

    // Text format suites (conformance_suite "text").
    {"TextAnyTest", ConformanceSection::kTextFormat_Extensions_Any},
    {"TextClosedEnumTest", ConformanceSection::kTextFormat_Fields_ClosedEnums},
    {"TextOpenEnumTest", ConformanceSection::kTextFormat_Fields_OpenEnums},
    {"TextDoubleTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextFloatInfinityTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextFloatLiteralTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextFloatNanTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextFloatTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextDelimitedTest", ConformanceSection::kTextFormat_Fields_Delimited},
    {"TextGroupTest", ConformanceSection::kTextFormat_Fields_Groups},
    {"TextHelloWorldTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextIntegerTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextMapTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextReservedFieldTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextBytesFieldTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextStringFieldTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextStringLiteralTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextFieldSeparatorTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextListSeparatorTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextNestedSeparatorTest", ConformanceSection::kTextFormat_Lexer_Literals},
    {"TextUnknownFieldTest", ConformanceSection::kTextFormat_Lexer_Literals},

    // JSON suites (conformance_suite "json").
    {"JsonAnyTest", ConformanceSection::kJson_Wkt_Any},
    {"JsonBoolTest", ConformanceSection::kJson_Primitives_NonRepeated},
    // The legacy RunJsonTestsForWrapperTypes() covered Duration and Timestamp.
    // TODO: b/564551336 - confirm section
    {"JsonDurationTest", ConformanceSection::kJson_Wkt_Wrappers},
    {"JsonTimestampTest", ConformanceSection::kJson_Wkt_Wrappers},
    {"JsonEnumTest", ConformanceSection::kJson_Primitives_NonRepeated},
    {"JsonProto3EnumTest", ConformanceSection::kJson_Primitives_NonRepeated},
    {"JsonFieldMaskTest", ConformanceSection::kJson_Wkt_FieldMask},
    {"JsonDuplicateFieldNameTest",
     ConformanceSection::kJson_FieldNames_Conventions},
    {"JsonFieldNameSerializationTest",
     ConformanceSection::kJson_FieldNames_Conventions},
    {"JsonFieldNameTest", ConformanceSection::kJson_FieldNames_Conventions},
    {"JsonObjectSyntaxTest", ConformanceSection::kJson_FieldNames_Conventions},
    // TODO: b/564551336 - confirm section
    {"JsonSkipsDefaultTest",
     ConformanceSection::kJson_Primitives_StoresDefaultPrimitive},
    {"JsonStoresDefaultTest",
     ConformanceSection::kJson_Primitives_StoresDefaultPrimitive},
    {"JsonDoubleTest", ConformanceSection::kJson_Primitives_NonRepeated},
    {"JsonFloatTest", ConformanceSection::kJson_Primitives_NonRepeated},
    // The ignore-unknown and top-level-null tests of this suite are refiled by
    // kTestSections below.
    {"JsonHelloWorldTest", ConformanceSection::kJson_FieldNames_HelloWorld},
    {"JsonIntegerTest", ConformanceSection::kJson_Primitives_NonRepeated},
    // TODO: b/564551336 - confirm section
    {"JsonMapTest", ConformanceSection::kJson_Primitives_NonRepeated},
    {"JsonMessageTest", ConformanceSection::kJson_Primitives_NonRepeated},
    {"JsonNullTest", ConformanceSection::kJson_Primitives_NullTypes},
    // TODO: b/564551336 - confirm section
    {"JsonOneofTest", ConformanceSection::kJson_Primitives_NonRepeated},
    {"JsonRepeatedTest", ConformanceSection::kJson_Primitives_Repeated},
    {"JsonBytesTest", ConformanceSection::kJson_Primitives_NonRepeated},
    {"JsonStringTest", ConformanceSection::kJson_Primitives_NonRepeated},
    {"JsonStructTest", ConformanceSection::kJson_Wkt_Struct},
    {"JsonUnknownEnumTest", ConformanceSection::kJson_Enums_UnknownEnumValues},
    {"JsonValueTest", ConformanceSection::kJson_Wkt_Value},
    {"JsonWrapperTest", ConformanceSection::kJson_Wkt_Wrappers},
};

using ::google::protobuf::conformance::internal::TestSectionOverride;

// Tests filed under a different section than the rest of their suite.
constexpr TestSectionOverride kTestSections[] = {
    // The legacy TestUnknownOrdering().
    {"UnknownFieldsTest", "UnknownOrdering",
     ConformanceSection::kInformational_UnknownFieldOrder},
    // The legacy map and oneof merge tests ran in the map and oneof sections.
    {"MergeTest", "MapMessageValue", ConformanceSection::kSemantics_Maps_Types},
    {"MergeTest", "MapMessageValueJson",
     ConformanceSection::kSemantics_Maps_Types},
    {"MergeTest", "OneofMessage", ConformanceSection::kSemantics_Oneofs_Types},
    {"MergeTest", "OneofMessageJson",
     ConformanceSection::kSemantics_Oneofs_Types},
    {"MergeTest", "OneofMessageBinary",
     ConformanceSection::kSemantics_Oneofs_Types},
    // The legacy RunJsonTestsForIgnoreUnknownAndReject().
    {"JsonHelloWorldTest", "IgnoreUnknownJsonNumber",
     ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject},
    {"JsonHelloWorldTest", "IgnoreUnknownJsonString",
     ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject},
    {"JsonHelloWorldTest", "IgnoreUnknownJsonTrue",
     ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject},
    {"JsonHelloWorldTest", "IgnoreUnknownJsonFalse",
     ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject},
    {"JsonHelloWorldTest", "IgnoreUnknownJsonNull",
     ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject},
    {"JsonHelloWorldTest", "IgnoreUnknownJsonObject",
     ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject},
    {"JsonHelloWorldTest", "RejectTopLevelNull",
     ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject},
};

// The suites whose section is refined by the field type they are
// parameterized over; see kFieldTypeSections.
constexpr absl::string_view kFieldTypeSuites[] = {
    "ValidDataScalarTest",
    "RepeatedScalarSelectsLastTest",
    "ValidDataRepeatedTest",
    "ValidDataRepeatedNonPackableTest",
};

struct FieldTypeSection {
  absl::string_view field_type;
  ConformanceSection section;
};

// The section of a valid-data test by the wire encoding of its field type,
// which is how the legacy TestValidDataFor*() calls were filed.  The field
// types are the ParamName(FieldDescriptor::Type) spellings.
constexpr FieldTypeSection kFieldTypeSections[] = {
    {"INT32", ConformanceSection::kWire_Varint_Integers},
    {"INT64", ConformanceSection::kWire_Varint_Integers},
    {"UINT32", ConformanceSection::kWire_Varint_Integers},
    {"UINT64", ConformanceSection::kWire_Varint_Integers},
    {"BOOL", ConformanceSection::kWire_Varint_BoolZigzag},
    {"SINT32", ConformanceSection::kWire_Varint_BoolZigzag},
    {"SINT64", ConformanceSection::kWire_Varint_BoolZigzag},
    {"ENUM", ConformanceSection::kWire_Varint_Enum},
    {"DOUBLE", ConformanceSection::kWire_Fixed_FloatingPoint},
    {"FLOAT", ConformanceSection::kWire_Fixed_FloatingPoint},
    {"FIXED32", ConformanceSection::kWire_Fixed_Integers},
    {"FIXED64", ConformanceSection::kWire_Fixed_Integers},
    {"SFIXED32", ConformanceSection::kWire_Fixed_Integers},
    {"SFIXED64", ConformanceSection::kWire_Fixed_Integers},
    {"STRING", ConformanceSection::kWire_LengthDelimited_Primitives},
    {"BYTES", ConformanceSection::kWire_LengthDelimited_Primitives},
    {"MESSAGE", ConformanceSection::kWire_LengthDelimited_Submessages},
};

// The gtest parameter components that are spelled in several cases (see
// ParamToSnakeCase()), in lowercase.
constexpr absl::string_view kCaseSensitiveLiterals[] = {
    "inf",
    "infinity",
    "nan",
};

const absl::flat_hash_map<absl::string_view, ConformanceSection>&
SuiteSectionMap() {
  static const absl::NoDestructor<
      absl::flat_hash_map<absl::string_view, ConformanceSection>>
      map([] {
        absl::flat_hash_map<absl::string_view, ConformanceSection> map;
        for (const SuiteSection& entry : kSuiteSections) {
          map.emplace(entry.suite, entry.section);
        }
        return map;
      }());
  return *map;
}

// The field-type refinement of `suite`'s section, or null if the suite isn't
// refined or `field_type` isn't a field type.
const SectionDescriptor* absl_nullable FieldTypeSectionOf(
    absl::string_view suite, absl::string_view field_type) {
  if (field_type.empty()) return nullptr;
  bool refined = false;
  for (absl::string_view candidate : kFieldTypeSuites) {
    if (candidate == suite) {
      refined = true;
      break;
    }
  }
  if (!refined) return nullptr;
  for (const FieldTypeSection& entry : kFieldTypeSections) {
    if (entry.field_type == field_type) {
      return &GetSectionDescriptor(entry.section);
    }
  }
  return nullptr;
}

struct SyntaxKeyword {
  absl::string_view param_component;
  absl::string_view tag;
  int digit;
};

constexpr SyntaxKeyword kSyntaxKeywords[] = {
    {"Proto2", "proto2", 1},
    {"Proto3", "proto3", 2},
    {"EditionsProto2", "ed_proto2", 3},
    {"EditionsProto3", "ed_proto3", 4},
    {"Editions", "ed2023", 5},
    {"EditionUnstable", "ed_next", 6},
};

// Index of a payload format within the transform grid, 1 based.
int FormatIndex(::conformance::WireFormat format) {
  switch (format) {
    case ::conformance::PROTOBUF:
      return 1;
    case ::conformance::JSON:
      return 2;
    case ::conformance::TEXT_FORMAT:
      return 3;
    default:
      return 0;
  }
}

::conformance::WireFormat InputFormat(const ConformanceRequest& request) {
  switch (request.payload_case()) {
    case ConformanceRequest::kProtobufPayload:
      return ::conformance::PROTOBUF;
    case ConformanceRequest::kJsonPayload:
      return ::conformance::JSON;
    case ConformanceRequest::kTextPayload:
      return ::conformance::TEXT_FORMAT;
    default:
      return ::conformance::UNSPECIFIED;
  }
}

}  // namespace

const SectionDescriptor& GetSectionDescriptor(ConformanceSection section) {
  switch (section) {
    // Domain 1: Wire Format
    case ConformanceSection::kWire_Varint_Integers: {
      static constexpr SectionDescriptor desc{1,        "wire", 1,
                                              "varint", 1,      "integers"};
      return desc;
    }
    case ConformanceSection::kWire_Varint_BoolZigzag: {
      static constexpr SectionDescriptor desc{1,        "wire", 1,
                                              "varint", 2,      "bool_zigzag"};
      return desc;
    }
    case ConformanceSection::kWire_Varint_Enum: {
      static constexpr SectionDescriptor desc{1,        "wire", 1,
                                              "varint", 3,      "enum"};
      return desc;
    }
    case ConformanceSection::kWire_Varint_Eof: {
      static constexpr SectionDescriptor desc{1, "wire", 1, "varint", 5, "eof"};
      return desc;
    }
    case ConformanceSection::kWire_Fixed_FloatingPoint: {
      static constexpr SectionDescriptor desc{1, "wire",          2, "fixed",
                                              1, "floating_point"};
      return desc;
    }
    case ConformanceSection::kWire_Fixed_Integers: {
      static constexpr SectionDescriptor desc{1,       "wire", 2,
                                              "fixed", 2,      "integers"};
      return desc;
    }
    case ConformanceSection::kWire_LengthDelimited_Utf8: {
      static constexpr SectionDescriptor desc{1, "wire", 3, "length_delimited",
                                              1, "utf8"};
      return desc;
    }
    case ConformanceSection::kWire_LengthDelimited_Primitives: {
      static constexpr SectionDescriptor desc{
          1, "wire", 3, "length_delimited", 2, "primitives"};
      return desc;
    }
    case ConformanceSection::kWire_LengthDelimited_Submessages: {
      static constexpr SectionDescriptor desc{
          1, "wire", 3, "length_delimited", 3, "submessages"};
      return desc;
    }
    case ConformanceSection::kWire_LengthDelimited_RecursionLimit: {
      static constexpr SectionDescriptor desc{
          1, "wire", 3, "length_delimited", 4, "recursion_limit"};
      return desc;
    }
    case ConformanceSection::kWire_GroupsUnknown_WireTypes: {
      static constexpr SectionDescriptor desc{
          1, "wire", 4, "groups_unknown", 1, "wire_types"};
      return desc;
    }
    case ConformanceSection::kWire_GroupsUnknown_UnknownFields: {
      static constexpr SectionDescriptor desc{
          1, "wire", 4, "groups_unknown", 2, "unknown_fields"};
      return desc;
    }
    case ConformanceSection::kWire_MessageSets: {
      static constexpr SectionDescriptor desc{
          1, "wire", 5, "message_sets", 1, "message_set"};
      return desc;
    }

    // Domain 2: TextFormat
    case ConformanceSection::kTextFormat_Lexer_Literals: {
      static constexpr SectionDescriptor desc{2, "text_format", 1, "lexer",
                                              1, "literals"};
      return desc;
    }
    case ConformanceSection::kTextFormat_Fields_Groups: {
      static constexpr SectionDescriptor desc{2, "text_format", 2, "fields",
                                              1, "groups"};
      return desc;
    }
    case ConformanceSection::kTextFormat_Fields_ClosedEnums: {
      static constexpr SectionDescriptor desc{2, "text_format", 2, "fields",
                                              2, "closed_enums"};
      return desc;
    }
    case ConformanceSection::kTextFormat_Fields_Delimited: {
      static constexpr SectionDescriptor desc{2, "text_format", 2, "fields",
                                              3, "delimited"};
      return desc;
    }
    case ConformanceSection::kTextFormat_Fields_OpenEnums: {
      static constexpr SectionDescriptor desc{2, "text_format", 2, "fields",
                                              4, "open_enums"};
      return desc;
    }
    case ConformanceSection::kTextFormat_Extensions_Any: {
      static constexpr SectionDescriptor desc{2, "text_format", 3, "extensions",
                                              1, "any"};
      return desc;
    }
    case ConformanceSection::kTextFormat_Performance_Merge: {
      static constexpr SectionDescriptor desc{
          2, "text_format", 4, "performance", 1, "merge"};
      return desc;
    }

    // Domain 3: JSON
    case ConformanceSection::kJson_FieldNames_HelloWorld: {
      static constexpr SectionDescriptor desc{3, "json",       1, "field_names",
                                              1, "hello_world"};
      return desc;
    }
    case ConformanceSection::kJson_FieldNames_Conventions: {
      static constexpr SectionDescriptor desc{3, "json",       1, "field_names",
                                              2, "conventions"};
      return desc;
    }
    case ConformanceSection::kJson_Primitives_NonRepeated: {
      static constexpr SectionDescriptor desc{3, "json",        2, "primitives",
                                              1, "non_repeated"};
      return desc;
    }
    case ConformanceSection::kJson_Primitives_Repeated: {
      static constexpr SectionDescriptor desc{3, "json",    2, "primitives",
                                              2, "repeated"};
      return desc;
    }
    case ConformanceSection::kJson_Primitives_NullTypes: {
      static constexpr SectionDescriptor desc{3, "json",      2, "primitives",
                                              3, "null_types"};
      return desc;
    }
    case ConformanceSection::kJson_Primitives_StoresDefaultPrimitive: {
      static constexpr SectionDescriptor desc{
          3, "json", 2, "primitives", 4, "stores_default_primitive"};
      return desc;
    }
    case ConformanceSection::kJson_Wkt_Wrappers: {
      static constexpr SectionDescriptor desc{3,     "json", 3,
                                              "wkt", 1,      "wrappers"};
      return desc;
    }
    case ConformanceSection::kJson_Wkt_FieldMask: {
      static constexpr SectionDescriptor desc{3,     "json", 3,
                                              "wkt", 2,      "field_mask"};
      return desc;
    }
    case ConformanceSection::kJson_Wkt_Struct: {
      static constexpr SectionDescriptor desc{3, "json", 3, "wkt", 3, "struct"};
      return desc;
    }
    case ConformanceSection::kJson_Wkt_Value: {
      static constexpr SectionDescriptor desc{3, "json", 3, "wkt", 4, "value"};
      return desc;
    }
    case ConformanceSection::kJson_Wkt_Any: {
      static constexpr SectionDescriptor desc{3, "json", 3, "wkt", 5, "any"};
      return desc;
    }
    case ConformanceSection::kJson_Enums_UnknownEnumValues: {
      static constexpr SectionDescriptor desc{
          3, "json", 4, "enums", 1, "unknown_enum_values"};
      return desc;
    }
    case ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject: {
      static constexpr SectionDescriptor desc{
          3, "json", 5, "parsing", 1, "ignore_unknown_and_reject"};
      return desc;
    }

    // Domain 4: Semantics
    case ConformanceSection::kSemantics_Oneofs_Types: {
      static constexpr SectionDescriptor desc{4, "semantics",  2, "oneofs",
                                              1, "oneof_types"};
      return desc;
    }
    case ConformanceSection::kSemantics_Oneofs_Unknown: {
      static constexpr SectionDescriptor desc{4, "semantics",    2, "oneofs",
                                              2, "oneof_unknown"};
      return desc;
    }
    case ConformanceSection::kSemantics_Maps_Types: {
      static constexpr SectionDescriptor desc{4,      "semantics", 3,
                                              "maps", 1,           "map_types"};
      return desc;
    }

    // Domain 5: Editions
    case ConformanceSection::kEditions_Delimited_Fields: {
      static constexpr SectionDescriptor desc{
          5, "editions", 1, "delimited", 1, "delimited_fields"};
      return desc;
    }
    case ConformanceSection::kEditions_Utf8_Validation: {
      static constexpr SectionDescriptor desc{5, "editions",       2, "utf8",
                                              1, "utf8_validation"};
      return desc;
    }
    case ConformanceSection::kEditions_Features_Unstable: {
      static constexpr SectionDescriptor desc{5, "editions", 3, "features",
                                              1, "unstable"};
      return desc;
    }

    // Domain 999: Informational
    case ConformanceSection::kInformational_UnknownFieldOrder: {
      static constexpr SectionDescriptor desc{
          999, "informational", 3,   "unknown_field_order",
          1,   "field_order",   true};
      return desc;
    }
    case ConformanceSection::kInformational_Performance_Benchmarks: {
      static constexpr SectionDescriptor desc{
          999, "informational", 4, "performance", 1, "benchmarks", true};
      return desc;
    }
  }
  static constexpr SectionDescriptor kUnknown{
      0, "uncategorized", 0, "unknown", 0, ""};
  return kUnknown;
}

const SectionDescriptor* absl_nullable SectionOf(absl::string_view suite,
                                                 absl::string_view test,
                                                 absl::string_view field_type) {
  if (!test.empty()) {
    for (const TestSectionOverride& entry : kTestSections) {
      if (entry.suite == suite && entry.test == test) {
        return &GetSectionDescriptor(entry.section);
      }
    }
  }
  if (const SectionDescriptor* refined = FieldTypeSectionOf(suite, field_type);
      refined != nullptr) {
    return refined;
  }
  const auto& map = SuiteSectionMap();
  auto it = map.find(suite);
  if (it != map.end()) return &GetSectionDescriptor(it->second);
  // An unlisted performance suite is a benchmark.
  if (absl::EndsWith(suite, "PerformanceTest")) {
    return &GetSectionDescriptor(
        ConformanceSection::kInformational_Performance_Benchmarks);
  }
  return nullptr;
}

std::string ToSnakeCase(absl::string_view token) {
  std::string result;
  result.reserve(token.size() + 8);
  for (size_t i = 0; i < token.size(); ++i) {
    const char c = token[i];
    if (!absl::ascii_isalnum(c)) {
      // Any run of punctuation collapses into a single separator.
      if (!result.empty() && result.back() != '_') {
        result.push_back('_');
      }
      continue;
    }
    if (absl::ascii_isupper(c) &&
        (i == 0 || !absl::ascii_isalnum(token[i - 1])) &&
        (i + 1 == token.size() || !absl::ascii_isalnum(token[i + 1]))) {
      if (!result.empty() && result.back() != '_') {
        result.push_back('_');
      }
      absl::StrAppend(&result, "upper_",
                      std::string(1, absl::ascii_tolower(c)));
      continue;
    }
    bool boundary = false;
    if (i > 0 && absl::ascii_isupper(c)) {
      const char prev = token[i - 1];
      if (absl::ascii_islower(prev) || absl::ascii_isdigit(prev)) {
        // "fooBar" / "int32Field" -> "foo_bar" / "int32_field".
        boundary = true;
      } else if (absl::ascii_isupper(prev) && i + 1 < token.size() &&
                 absl::ascii_islower(token[i + 1])) {
        // "JSONValue" -> "json_value".
        boundary = true;
      }
    }
    if (boundary && !result.empty() && result.back() != '_') {
      result.push_back('_');
    }
    result.push_back(absl::ascii_tolower(c));
  }
  while (!result.empty() && result.back() == '_') {
    result.pop_back();
  }
  return result;
}

std::string ParamToSnakeCase(absl::string_view component) {
  const std::string lower = absl::AsciiStrToLower(component);
  for (absl::string_view literal : kCaseSensitiveLiterals) {
    if (lower != literal) continue;
    if (component == lower) return absl::StrCat(literal, "_lower");
    if (component == absl::AsciiStrToUpper(component)) {
      return absl::StrCat(literal, "_upper");
    }
    // Mixed case: spell out which letters are upper case ("NaN" -> "ulu"),
    // since a suite may instantiate several mixed-case spellings ("NaN" and
    // "nAn", say).
    std::string mask;
    for (char c : component) {
      mask.push_back(absl::ascii_isupper(c) ? 'u' : 'l');
    }
    return absl::StrCat(literal, "_mixed_", mask);
  }
  return ToSnakeCase(component);
}

absl::string_view SyntaxTag(absl::string_view param_component) {
  for (const SyntaxKeyword& entry : kSyntaxKeywords) {
    if (entry.param_component == param_component) return entry.tag;
  }
  return "";
}

std::string PayloadTag(const ConformanceRequest& request) {
  const absl::string_view in = internal::FormatTag(InputFormat(request));
  const absl::string_view out =
      internal::FormatTag(request.requested_output_format());
  if (in.empty() || out.empty()) return "";
  return absl::StrCat(in, "2", out);
}

int TestNumberer::NumberFor(absl::string_view section_coordinate,
                            absl::string_view test_case) {
  const std::string key = absl::StrCat(section_coordinate, "|", test_case);
  auto [it, inserted] = test_numbers_.try_emplace(
      key, next_test_number_[std::string(section_coordinate)] + 1);
  if (inserted) ++next_test_number_[std::string(section_coordinate)];
  return it->second;
}

namespace internal {

int SyntaxDigit(absl::string_view syntax_tag) {
  for (const SyntaxKeyword& entry : kSyntaxKeywords) {
    if (entry.tag == syntax_tag) return entry.digit;
  }
  return 0;
}

absl::string_view FormatTag(::conformance::WireFormat format) {
  switch (format) {
    case ::conformance::PROTOBUF:
      return "pb";
    case ::conformance::JSON:
      return "json";
    case ::conformance::TEXT_FORMAT:
      return "text";
    default:
      return "";
  }
}

int PayloadDigit(const ConformanceRequest& request) {
  const int in = FormatIndex(InputFormat(request));
  const int out = FormatIndex(request.requested_output_format());
  if (in == 0 || out == 0) return 0;
  return (in - 1) * 3 + out;
}

absl::Span<const TestSectionOverride> TestSectionOverridesForTesting() {
  return kTestSections;
}

}  // namespace internal

}  // namespace conformance
}  // namespace protobuf
}  // namespace google
