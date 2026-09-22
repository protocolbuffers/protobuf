// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef GOOGLE_PROTOBUF_CONFORMANCE_TAXONOMY_H__
#define GOOGLE_PROTOBUF_CONFORMANCE_TAXONOMY_H__

#include <string>

#include "absl/base/nullability.h"
#include "absl/container/btree_map.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "conformance/conformance.pb.h"

// The coordinate taxonomy of the conformance tests.
//
// Every conformance test has, besides the gtest-derived name the failure lists
// use, a four-part identity, each part with a number and a name.  The two
// compose into parallel identifiers of the same shape:
//
//   coordinate   01.01.001.31
//   test_name    wire.varint.eof_before_unknown_value_double_parsefails
//                    .ed_proto2_pb2pb
//
//   part        number  name
//   ----------  ------  ------------------------------------------------
//   domain      01      wire                                } from the
//   section     01      varint                              } section table
//   test case   001     eof_before_unknown_value_double_parsefails
//   variant     31      ed_proto2_pb2pb
//
// The domain and section come from the gtest suite (see SectionOf()); the
// test case from the gtest test, its parameters and suffix; the variant from
// the message syntax and the payload formats.  See ResolveTestInfo() in
// testee.h for the derivation and ConformanceTestInfo for the result.
namespace google {
namespace protobuf {
namespace conformance {

enum class ConformanceSection {
  // Domain 1: Wire Format
  kWire_Varint_Integers,
  kWire_Varint_BoolZigzag,
  kWire_Varint_Enum,
  kWire_Varint_Eof,
  kWire_Fixed_FloatingPoint,
  kWire_Fixed_Integers,
  kWire_LengthDelimited_Utf8,
  kWire_LengthDelimited_Primitives,
  kWire_LengthDelimited_Submessages,
  kWire_LengthDelimited_RecursionLimit,
  kWire_GroupsUnknown_WireTypes,
  kWire_GroupsUnknown_UnknownFields,
  kWire_MessageSets,

  // Domain 2: TextFormat
  kTextFormat_Lexer_Literals,
  kTextFormat_Fields_Groups,
  kTextFormat_Fields_ClosedEnums,
  kTextFormat_Fields_Delimited,
  kTextFormat_Fields_OpenEnums,
  kTextFormat_Extensions_Any,
  kTextFormat_Performance_Merge,

  // Domain 3: JSON
  kJson_FieldNames_HelloWorld,
  kJson_FieldNames_Conventions,
  kJson_Primitives_NonRepeated,
  kJson_Primitives_Repeated,
  kJson_Primitives_NullTypes,
  kJson_Primitives_StoresDefaultPrimitive,
  kJson_Wkt_Wrappers,
  kJson_Wkt_FieldMask,
  kJson_Wkt_Struct,
  kJson_Wkt_Value,
  kJson_Wkt_Any,
  kJson_Enums_UnknownEnumValues,
  kJson_Parsing_IgnoreUnknownAndReject,

  // Domain 4: Semantics
  kSemantics_Oneofs_Types,
  kSemantics_Oneofs_Unknown,
  kSemantics_Maps_Types,

  // Domain 5: Editions
  kEditions_Delimited_Fields,
  kEditions_Utf8_Validation,
  kEditions_Features_Unstable,

  // Domain 999: Informational
  kInformational_UnknownFieldOrder,
  kInformational_Performance_Benchmarks,
};

// The first two parts of a test's identity, plus the subsection that prefixes
// its test case name.  The subsection is deliberately not a coordinate
// component: a section's test numbers run across all of its subsections.
struct SectionDescriptor {
  int domain_num;
  absl::string_view domain;
  int section_num;
  absl::string_view section;
  int subsection_num;
  absl::string_view subsection;
  // Informational sections document behavior that may legitimately vary
  // between implementations; a pass in one is a PASS_ALTERNATE.
  bool is_informational = false;
};

const SectionDescriptor& GetSectionDescriptor(ConformanceSection section);

// The section of a conformance gtest suite, or null if the suite isn't in the
// table.  `suite` is the suite name as gtest reports it, without the
// INSTANTIATE_TEST_SUITE_P prefix (e.g. "PrematureEofTest"), exactly what
// TestName::suite (testee.h) carries.  `test` is the gtest test name without
// the "/<params>" part; a few tests are filed under a different section than
// the rest of their suite (e.g. UnknownFieldsTest.UnknownOrdering is
// informational), so pass it when it is known.  `field_type` is the field type
// component of the test's parameters ("DOUBLE", "INT32", ...; see
// ParamName(FieldDescriptor::Type) in binary_test_util.h), if any: the wire
// format valid-data suites are parameterized by field type and are filed by
// the field's encoding, the way the legacy suite filed them (a DOUBLE test is
// a fixed floating-point test, a SINT32 test a bool/zigzag one, ...).  Any
// suite whose name ends in "PerformanceTest" and isn't listed is an
// informational benchmark.
//
// An unmapped suite is allowed at runtime (ResolveTestInfo() files it under
// domain 0, "unmapped"), but taxonomy_test checks that every suite linked into
// the conformance test binaries is mapped.
const SectionDescriptor* absl_nullable SectionOf(
    absl::string_view suite, absl::string_view test = "",
    absl::string_view field_type = "");

// Converts a CamelCase token into snake_case.
// "ValidDataScalar" -> "valid_data_scalar", "INT64[1]" -> "int64_1".
//
// A lone uppercase letter is case significant and is rendered as "upper_<c>":
// "FloatField_f" and "FloatField_F" are distinct tests exercising the "f" and
// "F" float literal suffixes, so plain lowercasing would merge them.
std::string ToSnakeCase(absl::string_view token);

// The snake_case form of a gtest parameter component.  Unlike suite and test
// names, which are C++ identifiers, parameters are free-form strings whose
// case can be all that tells two of them apart: the text format suites spell
// the special float values as "inf", "INF" and "iNF", say.  A component that
// is one of those literals (case-insensitively: inf, infinity, nan) gets a
// case marker: "<literal>_lower", "<literal>_upper" or, for a mixed-case
// spelling, "<literal>_mixed_<mask>" where the mask has a "u" or "l" per
// letter ("NaN" -> "nan_mixed_ulu", "iNF" -> "inf_mixed_luu"), so that every
// spelling a suite instantiates stays distinct.  Anything else is
// ToSnakeCase().
std::string ParamToSnakeCase(absl::string_view component);

// The compact syntax tag of a gtest parameter component, or "" if the
// component isn't a message syntax: "Proto2" -> "proto2", "Proto3" ->
// "proto3", "EditionsProto2" -> "ed_proto2", "EditionsProto3" -> "ed_proto3",
// "Editions" -> "ed2023", "EditionUnstable" -> "ed_next".  The components are
// the ParamName()s of GetEditionParamName() (naming.h).
absl::string_view SyntaxTag(absl::string_view param_component);

// The payload transform of a request, "<input>2<output>" (e.g. "pb2json"), or
// "" if either format is unknown.
std::string PayloadTag(const ::conformance::ConformanceRequest& request);

// Assigns test numbers (the third part of a coordinate).  Numbers are
// assigned per section in first-appearance order and memoized on the test
// case name, so that every variant of the same logical test shares a number,
// and a test that only exists for some syntaxes leaves a hole instead of
// shifting the numbering of everything that follows it.
//
// First-appearance order is that of one binary's run, so the numbers depend on
// which tests run and in what order (--gtest_filter, --gtest_shuffle, sharding
// and the set of suites linked in all change them); TestManager renumbers the
// exported results in sorted order before writing them (see BuildRunResult()
// in test_manager.h), which makes them independent of the run order but not
// of the set of tests run.
// TODO: b/564551336 - assign numbers from a checked-in table instead.
class TestNumberer {
 public:
  // Returns the number of `test_case` within `section_coordinate` ("01.01"),
  // assigning the next free one on first sight.
  int NumberFor(absl::string_view section_coordinate,
                absl::string_view test_case);

 private:
  // Key: "<section_coordinate>|<test_case>".
  absl::btree_map<std::string, int> test_numbers_;
  // Key: "<section_coordinate>".
  absl::btree_map<std::string, int> next_test_number_;
};

namespace internal {

// The tens digit of a variant: which syntax the test message uses, in the
// order of SyntaxTag()'s table (proto2 = 1 ... ed_next = 6), 0 for none.
int SyntaxDigit(absl::string_view syntax_tag);

// The compact tag of a payload format: "pb", "json" or "text"; "" for
// anything else.
absl::string_view FormatTag(::conformance::WireFormat format);

// The ones digit of a variant: which payload transform this is.
//
// A pure function of the 3x3 input/output grid, so the nine combinations map
// onto the nine nonzero digits exactly and a transform that no test currently
// exercises still owns a reserved number:
//
//   1 pb2pb    2 pb2json    3 pb2text
//   4 json2pb  5 json2json  6 json2text
//   7 text2pb  8 text2json  9 text2text
int PayloadDigit(const ::conformance::ConformanceRequest& request);

// A test filed under a different section than the rest of its suite; see
// SectionOf().
struct TestSectionOverride {
  absl::string_view suite;
  absl::string_view test;
  ConformanceSection section;
};

// The table of per-test overrides SectionOf() consults, so that taxonomy_test
// can check that every entry names a test that exists.
absl::Span<const TestSectionOverride> TestSectionOverridesForTesting();

}  // namespace internal

}  // namespace conformance
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_CONFORMANCE_TAXONOMY_H__
