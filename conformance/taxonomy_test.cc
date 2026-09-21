// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Tests of the coordinate taxonomy (taxonomy.h), and the checks that every
// conformance suite is in its section table and every per-test override
// names a real test.  For those this binary links every conformance suite
// library, which registers the suites with gtest; main() below filters them
// out so that only the Taxonomy* tests run (the suites would need an
// installed ConformanceEnvironment).  Don't run this binary with a broader
// --gtest_filter than that, e.g. "*": it would run the conformance suites
// themselves.

#include "conformance/taxonomy.h"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/container/flat_hash_set.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "conformance/conformance.pb.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

using ::google::protobuf::conformance::internal::FormatTag;
using ::google::protobuf::conformance::internal::PayloadDigit;
using ::google::protobuf::conformance::internal::SyntaxDigit;
using ::google::protobuf::conformance::internal::TestSectionOverride;
using ::google::protobuf::conformance::internal::TestSectionOverridesForTesting;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::IsNull;
using ::testing::Not;
using ::testing::NotNull;
using ::testing::Pointee;

TEST(TaxonomySectionDescriptorTest, CarriesTheOriginalNumbers) {
  const SectionDescriptor& eof =
      GetSectionDescriptor(ConformanceSection::kWire_Varint_Eof);
  EXPECT_EQ(eof.domain_num, 1);
  EXPECT_EQ(eof.domain, "wire");
  EXPECT_EQ(eof.section_num, 1);
  EXPECT_EQ(eof.section, "varint");
  EXPECT_EQ(eof.subsection_num, 5);
  EXPECT_EQ(eof.subsection, "eof");
  EXPECT_FALSE(eof.is_informational);

  const SectionDescriptor& text =
      GetSectionDescriptor(ConformanceSection::kTextFormat_Fields_OpenEnums);
  EXPECT_EQ(text.domain_num, 2);
  EXPECT_EQ(text.domain, "text_format");
  EXPECT_EQ(text.section_num, 2);
  EXPECT_EQ(text.section, "fields");
  EXPECT_EQ(text.subsection_num, 4);
  EXPECT_EQ(text.subsection, "open_enums");

  const SectionDescriptor& json = GetSectionDescriptor(
      ConformanceSection::kJson_Parsing_IgnoreUnknownAndReject);
  EXPECT_EQ(json.domain_num, 3);
  EXPECT_EQ(json.section_num, 5);
  EXPECT_EQ(json.subsection, "ignore_unknown_and_reject");

  const SectionDescriptor& semantics =
      GetSectionDescriptor(ConformanceSection::kSemantics_Maps_Types);
  EXPECT_EQ(semantics.domain_num, 4);
  EXPECT_EQ(semantics.section_num, 3);

  const SectionDescriptor& editions =
      GetSectionDescriptor(ConformanceSection::kEditions_Features_Unstable);
  EXPECT_EQ(editions.domain_num, 5);
  EXPECT_EQ(editions.section_num, 3);

  const SectionDescriptor& informational = GetSectionDescriptor(
      ConformanceSection::kInformational_Performance_Benchmarks);
  EXPECT_EQ(informational.domain_num, 999);
  EXPECT_EQ(informational.domain, "informational");
  EXPECT_EQ(informational.section_num, 4);
  EXPECT_TRUE(informational.is_informational);
}

TEST(TaxonomySectionOfTest, MapsSuitesByTopic) {
  EXPECT_THAT(SectionOf("PrematureEofTest"),
              Pointee(Field(&SectionDescriptor::subsection, Eq("eof"))));
  EXPECT_THAT(SectionOf("PrematureEofInPackedFieldTest"),
              Pointee(Field(&SectionDescriptor::subsection, Eq("eof"))));
  EXPECT_THAT(SectionOf("MessageSetTest"),
              Pointee(Field(&SectionDescriptor::section, Eq("message_sets"))));
  EXPECT_THAT(SectionOf("JsonDuplicateFieldNameTest"),
              Pointee(Field(&SectionDescriptor::section, Eq("field_names"))));
  EXPECT_THAT(SectionOf("TextOpenEnumTest"),
              Pointee(Field(&SectionDescriptor::subsection, Eq("open_enums"))));
  EXPECT_THAT(SectionOf("ValidDataMapTest"),
              Pointee(Field(&SectionDescriptor::section, Eq("maps"))));
  EXPECT_THAT(SectionOf("DelimitedFieldTest"),
              Pointee(Field(&SectionDescriptor::domain, Eq("editions"))));
}

TEST(TaxonomySectionOfTest, BenchmarkSuitesAreInformational) {
  for (absl::string_view suite :
       {"MergeMessagePerformanceTest", "UnknownFieldsPerformanceTest",
        "TextPerformanceTest"}) {
    EXPECT_THAT(
        SectionOf(suite),
        Pointee(AllOf(Field(&SectionDescriptor::domain_num, Eq(999)),
                      Field(&SectionDescriptor::subsection, Eq("benchmarks")),
                      Field(&SectionDescriptor::is_informational, Eq(true)))))
        << suite;
  }
}

TEST(TaxonomySectionOfTest, RecursionLimitSuitesAreRequiredWireTests) {
  for (absl::string_view suite : {"RecursionLimitPerformanceTest",
                                  "Edition2023RecursionLimitPerformanceTest"}) {
    EXPECT_THAT(
        SectionOf(suite),
        Pointee(
            AllOf(Field(&SectionDescriptor::domain, Eq("wire")),
                  Field(&SectionDescriptor::section, Eq("length_delimited")),
                  Field(&SectionDescriptor::subsection, Eq("recursion_limit")),
                  Field(&SectionDescriptor::is_informational, Eq(false)))))
        << suite;
  }
}

TEST(TaxonomySectionOfTest, ValidDataSuitesAreFiledByFieldType) {
  struct Case {
    absl::string_view field_type;
    absl::string_view section;
    absl::string_view subsection;
  };
  const Case kCases[] = {
      {"INT32", "varint", "integers"},
      {"INT64", "varint", "integers"},
      {"UINT32", "varint", "integers"},
      {"UINT64", "varint", "integers"},
      {"BOOL", "varint", "bool_zigzag"},
      {"SINT32", "varint", "bool_zigzag"},
      {"SINT64", "varint", "bool_zigzag"},
      {"ENUM", "varint", "enum"},
      {"DOUBLE", "fixed", "floating_point"},
      {"FLOAT", "fixed", "floating_point"},
      {"FIXED32", "fixed", "integers"},
      {"FIXED64", "fixed", "integers"},
      {"SFIXED32", "fixed", "integers"},
      {"SFIXED64", "fixed", "integers"},
      {"STRING", "length_delimited", "primitives"},
      {"BYTES", "length_delimited", "primitives"},
      {"MESSAGE", "length_delimited", "submessages"},
  };
  for (absl::string_view suite :
       {"ValidDataScalarTest", "ValidDataRepeatedTest",
        "RepeatedScalarSelectsLastTest", "ValidDataRepeatedNonPackableTest"}) {
    for (const Case& c : kCases) {
      EXPECT_THAT(SectionOf(suite, "Scalar", c.field_type),
                  Pointee(AllOf(
                      Field(&SectionDescriptor::domain, Eq("wire")),
                      Field(&SectionDescriptor::section, Eq(c.section)),
                      Field(&SectionDescriptor::subsection, Eq(c.subsection)))))
          << suite << " " << c.field_type;
    }
  }
  // Without a field type, or with something that isn't one, the suite's own
  // entry applies.
  EXPECT_THAT(SectionOf("ValidDataScalarTest"),
              Pointee(Field(&SectionDescriptor::subsection, Eq("integers"))));
  EXPECT_THAT(SectionOf("ValidDataScalarTest", "Scalar", "2"),
              Pointee(Field(&SectionDescriptor::subsection, Eq("integers"))));
  // Only the valid-data suites are refined; a field type in another suite's
  // parameters means nothing.
  EXPECT_THAT(
      SectionOf("PrematureEofTest", "BeforeKnownNonRepeatedValue", "DOUBLE"),
      Pointee(Field(&SectionDescriptor::subsection, Eq("eof"))));
}

TEST(TaxonomySectionOfTest, SomeTestsAreFiledAwayFromTheirSuite) {
  // The suite is about unknown fields; the ordering test is informational.
  EXPECT_THAT(
      SectionOf("UnknownFieldsTest", "UnknownVarint"),
      Pointee(Field(&SectionDescriptor::subsection, Eq("unknown_fields"))));
  EXPECT_THAT(SectionOf("UnknownFieldsTest", "UnknownOrdering"),
              Pointee(AllOf(Field(&SectionDescriptor::domain_num, Eq(999)),
                            Field(&SectionDescriptor::section,
                                  Eq("unknown_field_order")))));
  // Without the test name the suite's section is returned.
  EXPECT_THAT(
      SectionOf("UnknownFieldsTest"),
      Pointee(Field(&SectionDescriptor::subsection, Eq("unknown_fields"))));

  EXPECT_THAT(
      SectionOf("JsonHelloWorldTest", "HelloWorld"),
      Pointee(Field(&SectionDescriptor::subsection, Eq("hello_world"))));
  EXPECT_THAT(SectionOf("JsonHelloWorldTest", "RejectTopLevelNull"),
              Pointee(Field(&SectionDescriptor::section, Eq("parsing"))));
  EXPECT_THAT(SectionOf("MergeTest", "MapMessageValueJson"),
              Pointee(Field(&SectionDescriptor::section, Eq("maps"))));
}

TEST(TaxonomySectionOfTest, UnknownSuiteIsNull) {
  EXPECT_THAT(SectionOf("NoSuchTest"), IsNull());
  EXPECT_THAT(SectionOf("NoSuchTest", "UnknownOrdering"), IsNull());
  EXPECT_THAT(SectionOf(""), IsNull());
  // The INSTANTIATE prefix is not part of a suite name.
  EXPECT_THAT(SectionOf("All/PrematureEofTest"), IsNull());
}

TEST(TaxonomySectionOfTest, UnlistedPerformanceSuiteIsStillABenchmark) {
  EXPECT_THAT(
      SectionOf("BrandNewPerformanceTest"),
      Pointee(AllOf(Field(&SectionDescriptor::domain, "informational"),
                    Field(&SectionDescriptor::subsection, "benchmarks"),
                    Field(&SectionDescriptor::is_informational, true))));
  EXPECT_THAT(SectionOf("PerformanceTestOfSomething"), IsNull());
}

// Every gtest suite registered in this binary, without the
// INSTANTIATE_TEST_SUITE_P prefix ("All/FooTest" -> "FooTest"), as TestName
// (testee.h) spells suites.
std::vector<std::string> RegisteredConformanceSuites() {
  std::vector<std::string> suites;
  const testing::UnitTest& unit_test = *testing::UnitTest::GetInstance();
  for (int i = 0; i < unit_test.total_test_suite_count(); ++i) {
    absl::string_view name = unit_test.GetTestSuite(i)->name();
    if (absl::StartsWith(name, "Taxonomy")) continue;
    if (size_t slash = name.find('/'); slash != absl::string_view::npos) {
      name = name.substr(slash + 1);
    }
    suites.push_back(std::string(name));
  }
  return suites;
}

// Every "<Suite>.<Test>" registered in this binary, spelled like the suites
// above and with the "/<params>" part of the test dropped.
absl::flat_hash_set<std::string> RegisteredConformanceTests() {
  absl::flat_hash_set<std::string> tests;
  const testing::UnitTest& unit_test = *testing::UnitTest::GetInstance();
  for (int i = 0; i < unit_test.total_test_suite_count(); ++i) {
    const testing::TestSuite& suite = *unit_test.GetTestSuite(i);
    absl::string_view suite_name = suite.name();
    if (absl::StartsWith(suite_name, "Taxonomy")) continue;
    if (size_t slash = suite_name.find('/'); slash != absl::string_view::npos) {
      suite_name = suite_name.substr(slash + 1);
    }
    for (int j = 0; j < suite.total_test_count(); ++j) {
      absl::string_view test_name = suite.GetTestInfo(j)->name();
      test_name = test_name.substr(0, test_name.find('/'));
      tests.insert(absl::StrCat(suite_name, ".", test_name));
    }
  }
  return tests;
}

TEST(TaxonomyCoverageTest, EveryLinkedConformanceSuiteIsMapped) {
  std::vector<std::string> suites = RegisteredConformanceSuites();
  // This binary links the binary, json, text and performance suites; if none
  // registered, the check below would pass vacuously.
  ASSERT_GT(suites.size(), 50) << absl::StrJoin(suites, ", ");

  std::vector<std::string> unmapped;
  for (const std::string& suite : suites) {
    if (SectionOf(suite) == nullptr) unmapped.push_back(suite);
  }
  EXPECT_THAT(unmapped, IsEmpty())
      << "Add these suites to kSuiteSections in taxonomy.cc: "
      << absl::StrJoin(unmapped, ", ");
}

TEST(TaxonomyCoverageTest, OnlyPerformanceSuitesAreBenchmarks) {
  // The naming rule of test_environment.h: a *PerformanceTest fixture belongs
  // to the performance suite.  The taxonomy files most of those as
  // benchmarks (a few check required behavior, see kSuiteSections), and
  // nothing else as a benchmark.
  int benchmarks = 0;
  for (const std::string& suite : RegisteredConformanceSuites()) {
    const SectionDescriptor* section = SectionOf(suite);
    if (section == nullptr) continue;
    if (section->subsection != "benchmarks") continue;
    ++benchmarks;
    EXPECT_TRUE(absl::EndsWith(suite, "PerformanceTest")) << suite;
  }
  EXPECT_GT(benchmarks, 0);
}

TEST(TaxonomyCoverageTest, EveryTestOverrideNamesARegisteredTest) {
  const absl::flat_hash_set<std::string> tests = RegisteredConformanceTests();
  ASSERT_GT(tests.size(), 100);
  ASSERT_THAT(TestSectionOverridesForTesting(), Not(IsEmpty()));
  for (const TestSectionOverride& entry : TestSectionOverridesForTesting()) {
    EXPECT_TRUE(tests.contains(absl::StrCat(entry.suite, ".", entry.test)))
        << "kTestSections in taxonomy.cc names " << entry.suite << "."
        << entry.test << ", which isn't a registered test";
  }
}

TEST(TaxonomySnakeCaseTest, ConvertsCamelCase) {
  EXPECT_EQ(ToSnakeCase("ValidDataScalar"), "valid_data_scalar");
  EXPECT_EQ(ToSnakeCase("BeforeKnownNonRepeatedValue"),
            "before_known_non_repeated_value");
  EXPECT_EQ(ToSnakeCase("fooBar"), "foo_bar");
  EXPECT_EQ(ToSnakeCase("int32Field"), "int32_field");
  EXPECT_EQ(ToSnakeCase("JSONValue"), "json_value");
  EXPECT_EQ(ToSnakeCase("INT64"), "int64");
  EXPECT_EQ(ToSnakeCase("INT64[1]"), "int64_1");
  EXPECT_EQ(ToSnakeCase("DOUBLE"), "double");
  EXPECT_EQ(ToSnakeCase("PrematureEofTest"), "premature_eof_test");
}

TEST(TaxonomySnakeCaseTest, CollapsesPunctuation) {
  EXPECT_EQ(ToSnakeCase("a__b"), "a_b");
  EXPECT_EQ(ToSnakeCase("_a_"), "a");
  EXPECT_EQ(ToSnakeCase(""), "");
  EXPECT_EQ(ToSnakeCase("__"), "");
}

TEST(TaxonomySnakeCaseTest, LoneUppercaseLetterIsCaseSignificant) {
  EXPECT_EQ(ToSnakeCase("FloatField_f"), "float_field_f");
  EXPECT_EQ(ToSnakeCase("FloatField_F"), "float_field_upper_f");
  EXPECT_EQ(ToSnakeCase("F"), "upper_f");
}

TEST(TaxonomyParamSnakeCaseTest, SpecialFloatSpellingsGetACaseMarker) {
  // The spellings TextFloatInfinityTest and TextFloatNanTest instantiate.
  EXPECT_EQ(ParamToSnakeCase("inf"), "inf_lower");
  EXPECT_EQ(ParamToSnakeCase("INF"), "inf_upper");
  EXPECT_EQ(ParamToSnakeCase("iNF"), "inf_mixed_luu");
  EXPECT_EQ(ParamToSnakeCase("infinity"), "infinity_lower");
  EXPECT_EQ(ParamToSnakeCase("INFINITY"), "infinity_upper");
  EXPECT_EQ(ParamToSnakeCase("inFINITY"), "infinity_mixed_lluuuuuu");
  EXPECT_EQ(ParamToSnakeCase("nan"), "nan_lower");
  EXPECT_EQ(ParamToSnakeCase("NaN"), "nan_mixed_ulu");
  EXPECT_EQ(ParamToSnakeCase("nAn"), "nan_mixed_lul");
  EXPECT_EQ(ParamToSnakeCase("NAN"), "nan_upper");
}

TEST(TaxonomyParamSnakeCaseTest, EverythingElseIsPlainSnakeCase) {
  EXPECT_EQ(ParamToSnakeCase("INT32"), "int32");
  EXPECT_EQ(ParamToSnakeCase("string"), "string");
  EXPECT_EQ(ParamToSnakeCase("Infinite"), "infinite");
  EXPECT_EQ(ParamToSnakeCase("NanBoxed"), "nan_boxed");
  EXPECT_EQ(ParamToSnakeCase("f"), "f");
  EXPECT_EQ(ParamToSnakeCase("F"), "upper_f");
  EXPECT_EQ(ParamToSnakeCase("2"), "2");
  EXPECT_EQ(ParamToSnakeCase(""), "");
}

TEST(TaxonomySyntaxTest, TagsAndDigits) {
  EXPECT_EQ(SyntaxTag("Proto2"), "proto2");
  EXPECT_EQ(SyntaxTag("Proto3"), "proto3");
  EXPECT_EQ(SyntaxTag("EditionsProto2"), "ed_proto2");
  EXPECT_EQ(SyntaxTag("EditionsProto3"), "ed_proto3");
  EXPECT_EQ(SyntaxTag("Editions"), "ed2023");
  EXPECT_EQ(SyntaxTag("EditionUnstable"), "ed_next");
  EXPECT_EQ(SyntaxTag("INT32"), "");
  EXPECT_EQ(SyntaxTag(""), "");

  EXPECT_EQ(SyntaxDigit("proto2"), 1);
  EXPECT_EQ(SyntaxDigit("proto3"), 2);
  EXPECT_EQ(SyntaxDigit("ed_proto2"), 3);
  EXPECT_EQ(SyntaxDigit("ed_proto3"), 4);
  EXPECT_EQ(SyntaxDigit("ed2023"), 5);
  EXPECT_EQ(SyntaxDigit("ed_next"), 6);
  EXPECT_EQ(SyntaxDigit(""), 0);
  EXPECT_EQ(SyntaxDigit("Proto2"), 0);
}

TEST(TaxonomyPayloadTest, FormatTags) {
  EXPECT_EQ(FormatTag(::conformance::PROTOBUF), "pb");
  EXPECT_EQ(FormatTag(::conformance::JSON), "json");
  EXPECT_EQ(FormatTag(::conformance::TEXT_FORMAT), "text");
  EXPECT_EQ(FormatTag(::conformance::UNSPECIFIED), "");
  EXPECT_EQ(FormatTag(::conformance::JSPB), "");
}

TEST(TaxonomyPayloadTest, TheNineTransformsOwnTheNineDigits) {
  const ::conformance::WireFormat formats[] = {
      ::conformance::PROTOBUF, ::conformance::JSON, ::conformance::TEXT_FORMAT};
  const absl::string_view tags[] = {"pb", "json", "text"};
  int expected_digit = 1;
  for (int in = 0; in < 3; ++in) {
    for (int out = 0; out < 3; ++out) {
      EXPECT_EQ(PayloadDigit(formats[in], formats[out]), expected_digit)
          << tags[in] << tags[out];
      EXPECT_EQ(PayloadTag(formats[in], formats[out]),
                absl::StrCat(tags[in], "2", tags[out]));
      ++expected_digit;
    }
  }
}

TEST(TaxonomyPayloadTest, UnknownFormatsHaveNoDigit) {
  EXPECT_EQ(PayloadDigit(::conformance::UNSPECIFIED, ::conformance::PROTOBUF),
            0);
  EXPECT_EQ(PayloadTag(::conformance::UNSPECIFIED, ::conformance::PROTOBUF),
            "");
  EXPECT_EQ(PayloadDigit(::conformance::PROTOBUF, ::conformance::UNSPECIFIED),
            0);
  EXPECT_EQ(PayloadDigit(::conformance::PROTOBUF, ::conformance::JSPB), 0);
  EXPECT_EQ(PayloadTag(::conformance::PROTOBUF, ::conformance::JSPB), "");
}

TEST(TaxonomyTestNumbererTest, NumbersPerSectionInFirstAppearanceOrder) {
  TestNumberer numberer;
  EXPECT_EQ(numberer.NumberFor("01.01", "eof_a"), 1);
  EXPECT_EQ(numberer.NumberFor("01.01", "eof_b"), 2);
  // The same test case in another section starts that section's numbering.
  EXPECT_EQ(numberer.NumberFor("01.02", "eof_a"), 1);
  // Every variant of a test case shares its number.
  EXPECT_EQ(numberer.NumberFor("01.01", "eof_a"), 1);
  EXPECT_EQ(numberer.NumberFor("01.01", "eof_b"), 2);
  EXPECT_EQ(numberer.NumberFor("01.01", "eof_c"), 3);
  EXPECT_EQ(numberer.NumberFor("01.02", "eof_b"), 2);
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  // The linked conformance suites are only here to be enumerated; running
  // them would need an installed ConformanceEnvironment.
  if (GTEST_FLAG_GET(filter) == "*") GTEST_FLAG_SET(filter, "Taxonomy*");
  return RUN_ALL_TESTS();
}
