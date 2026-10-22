// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/testing/protobuf_test.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "google/protobuf/unittest.pb.h"

namespace google {
namespace protobuf {
namespace internal {
namespace {

using ::proto2_unittest::FOREIGN_BAR;
using ::proto2_unittest::FOREIGN_BAZ;
using ::proto2_unittest::FOREIGN_FOO;
using ::proto2_unittest::ForeignEnum;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Pair;
using ::testing::StrEq;
using ::testing::UnorderedElementsAre;

class ProtobufTestFixture : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    EXPECT_EQ(set_up_suite_count_, 0);
    EXPECT_EQ(set_up_count_, 0);
    ++set_up_suite_count_;
  }

  static void TearDownTestSuite() {
    EXPECT_EQ(set_up_suite_count_, 1);
    EXPECT_EQ(tear_down_suite_count_, 0);
    EXPECT_EQ(regular_test_f_runs_, 1);
    EXPECT_EQ(no_param_runs_, 1);
    EXPECT_EQ(set_up_count_, 1 + 1 + 3 + 6 + 3 + 4 + 2);
    EXPECT_EQ(tear_down_count_, 1 + 1 + 3 + 6 + 3 + 4 + 2);
    EXPECT_THAT(state().single_dim_values, ElementsAre(10, 20, 30));
    EXPECT_THAT(state().multi_dim_values,
                ElementsAre(Pair(1, "foo"), Pair(2, "foo"), Pair(1, "bar"),
                            Pair(2, "bar"), Pair(1, "baz"), Pair(2, "baz")));
    EXPECT_THAT(state().single_type_names,
                ElementsAre("int32_t", "double", "bool"));
    EXPECT_THAT(state().type_and_value_runs,
                ElementsAre(Pair(10, "int32_t"), Pair(20, "int32_t"),
                            Pair(10, "double"), Pair(20, "double")));
    EXPECT_THAT(
        state().non_static_fixture_values,
        ElementsAre(Pair(42, "NonStaticFixtureMember/fixture_state"),
                    Pair(52, "NonStaticFixtureMember/fixture_state_10")));
    ++tear_down_suite_count_;
  }

  struct State {
    std::vector<int> single_dim_values;
    std::vector<std::pair<int, std::string>> multi_dim_values;
    std::vector<std::string> single_type_names;
    std::vector<std::pair<int, std::string>> type_and_value_runs;
    std::vector<std::pair<int, std::string>> non_static_fixture_values;
  };

  static State& state() {
    static auto* const s = new State();
    return *s;
  }

  static inline int set_up_suite_count_ = 0;
  static inline int tear_down_suite_count_ = 0;
  static inline int set_up_count_ = 0;
  static inline int tear_down_count_ = 0;
  static inline int regular_test_f_runs_ = 0;
  static inline int no_param_runs_ = 0;

 protected:
  void SetUp() override {
    EXPECT_EQ(set_up_suite_count_, 1);
    EXPECT_EQ(tear_down_suite_count_, 0);
    fixture_state_ = 42;
    ++set_up_count_;
  }

  void TearDown() override { ++tear_down_count_; }

  int fixture_state_ = 0;
};

TEST_F(ProtobufTestFixture, RegularTestF) {
  EXPECT_EQ(fixture_state_, 42);
  ++regular_test_f_runs_;
}

PB_TEST_F(ProtobufTestFixture, NoParameters) {
  EXPECT_EQ(fixture_state_, 42);
  ++no_param_runs_;
  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_STREQ(test_info->name(), "NoParameters");
  EXPECT_EQ(test_info->value_param(), nullptr);
  EXPECT_EQ(test_info->type_param(), nullptr);
}

PB_TEST_F(ProtobufTestFixture, SingleDimension) {
  EXPECT_EQ(fixture_state_, 42);
  int v = PB_TEST_CHOOSE_VALUE((10, 20, 30));
  state().single_dim_values.push_back(v);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(), absl::StrCat("SingleDimension/", v));
  EXPECT_THAT(test_info->value_param(), StrEq(absl::StrCat(v)));
  EXPECT_EQ(test_info->type_param(), nullptr);
}

PB_TEST_F(ProtobufTestFixture, MultiDimension) {
  EXPECT_EQ(fixture_state_, 42);
  int a = PB_TEST_CHOOSE_VALUE((1, 2));
  absl::string_view b = PB_TEST_CHOOSE_VALUE(("foo", "bar", "baz"));
  state().multi_dim_values.emplace_back(a, std::string(b));

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(), absl::StrCat("MultiDimension/", a, "/", b));
  EXPECT_THAT(test_info->value_param(), StrEq(absl::StrCat(a, "/", b)));
  EXPECT_EQ(test_info->type_param(), nullptr);
}

PB_TEST_F(ProtobufTestFixture, SingleType) {
  EXPECT_EQ(fixture_state_, 42);
  using T = PB_TEST_CHOOSE_TYPE((int, double, bool));
  std::string type_name = *GetTypeName<T>();
  state().single_type_names.push_back(type_name);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(), absl::StrCat("SingleType/", type_name));
  EXPECT_EQ(test_info->value_param(), nullptr);
  EXPECT_THAT(test_info->type_param(), StrEq(type_name));
}

PB_TEST_F(ProtobufTestFixture, TypeAndValue) {
  EXPECT_EQ(fixture_state_, 42);
  int v = PB_TEST_CHOOSE_VALUE((10, 20));
  using T = PB_TEST_CHOOSE_TYPE((int, double));
  std::string type_name = *GetTypeName<T>();
  state().type_and_value_runs.emplace_back(v, type_name);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(),
            absl::StrCat("TypeAndValue/", v, "/", type_name));
  EXPECT_THAT(test_info->value_param(), StrEq(absl::StrCat(v)));
  EXPECT_THAT(test_info->type_param(), StrEq(type_name));
}

PB_TEST_F(ProtobufTestFixture, NonStaticFixtureMember) {
  int v = PB_TEST_CHOOSE_VALUE((fixture_state_, fixture_state_ + 10));
  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  state().non_static_fixture_values.emplace_back(v, test_info->name());
}

std::vector<bool>& RecordedBools() {
  static auto* v = new std::vector<bool>();
  return *v;
}

std::vector<ForeignEnum>& RecordedEnums() {
  static auto* v = new std::vector<ForeignEnum>();
  return *v;
}

std::vector<std::string>& RecordedSanitizedInputs() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedDupeInputs() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::pair<int, int>>& RecordedConditionalInputs() {
  static auto* v = new std::vector<std::pair<int, int>>();
  return *v;
}

std::vector<int>& RecordedSameLineSums() {
  static auto* v = new std::vector<int>();
  return *v;
}

std::vector<std::string>& RecordedMultiTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedTypesAndValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedTypeDependentValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedTypeDependentTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedDupeTypeTestNames() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedSanitizedTypeTestNames() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedSameLineTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedUnprintableTypeTestNames() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedConditionalTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedCustomPrintedValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedExplicitValueNames() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedUnprintableValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedNonStaticValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedPrefixedValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedCustomPrintedTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

#if __cplusplus >= 202002L
std::vector<std::string>& RecordedExplicitTypeNames() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedPrefixedTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}
#endif  // __cplusplus >= 202002L

std::vector<std::string>& RecordedTupleTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

int g_macro_first_runs = 0;
int g_macro_second_runs = 0;

PB_TEST(ProtobufStandaloneTest, ThreeDimensions) {
  int x = PB_TEST_CHOOSE_VALUE((1, 2));
  absl::string_view y = PB_TEST_CHOOSE_VALUE(("a", "b"));
  int z = PB_TEST_CHOOSE_VALUE((100, 200));
  EXPECT_TRUE(x == 1 || x == 2);
  EXPECT_TRUE(y == "a" || y == "b");
  EXPECT_TRUE(z == 100 || z == 200);
  EXPECT_EQ(::testing::UnitTest::GetInstance()->current_test_info()->name(),
            absl::StrCat("ThreeDimensions/", x, "/", y, "/", z));
}

PB_TEST(ProtobufStandaloneTest, BoolValues) {
  const bool& b = PB_TEST_CHOOSE_VALUE((false, true));
  RecordedBools().push_back(b);
  EXPECT_EQ(::testing::UnitTest::GetInstance()->current_test_info()->name(),
            absl::StrCat("BoolValues/", b ? "true" : "false"));
}

PB_TEST(ProtobufStandaloneTest, ProtoEnumValues) {
  ForeignEnum e = PB_TEST_CHOOSE_VALUE((FOREIGN_FOO, FOREIGN_BAR, FOREIGN_BAZ,
                                        static_cast<ForeignEnum>(999),
                                        static_cast<ForeignEnum>(-1)));
  RecordedEnums().push_back(e);
}

PB_TEST(ProtobufStandaloneTest, SanitizesNames) {
  absl::string_view s =
      PB_TEST_CHOOSE_VALUE(("", "///", "__a//b--", "foo-bar", "hello world!"));
  int n = PB_TEST_CHOOSE_VALUE((-5, 5, 10));
  RecordedSanitizedInputs().push_back(absl::StrCat(s, ":", n));
}

PB_TEST(ProtobufStandaloneTest, DisambiguatesDuplicateNames) {
  absl::string_view s =
      PB_TEST_CHOOSE_VALUE(("", "_", "a_b", "a-b", "a/b", "a b"));
  RecordedDupeInputs().emplace_back(s);
}

PB_TEST(ProtobufStandaloneTest, ConditionalChooseValue) {
  int a = PB_TEST_CHOOSE_VALUE((1, 2));
  if (a != 1) {
    GTEST_SKIP() << "Skipping early before second PB_TEST_CHOOSE_VALUE";
  }
  int b = PB_TEST_CHOOSE_VALUE((10, 20));
  RecordedConditionalInputs().emplace_back(a, b);
}

PB_TEST(ProtobufStandaloneTest, SameLineChooseValue) {
  int v = PB_TEST_CHOOSE_VALUE((1, 2)) + PB_TEST_CHOOSE_VALUE((10, 20));
  RecordedSameLineSums().push_back(v);
}

struct UnprintableValue {
  int a;
  int b;
};

std::string CustomIntPrinter(int val, size_t) {
  return val == 1 ? "first-val" : "first/val";
}

PB_TEST(ProtobufStandaloneTest, UnprintableValuesUseIndex) {
  static constexpr UnprintableValue kItems[] = {{1, 2}, {3, 4}};
  UnprintableValue v = PB_TEST_CHOOSE_VALUE(
      (UnprintableValue{10, 20}, UnprintableValue{30, 40}));
  UnprintableValue prefixed = PB_TEST_CHOOSE_VALUE(kItems, "item");
  RecordedUnprintableValues().push_back(absl::StrCat(
      v.a + v.b, ":", prefixed.a + prefixed.b, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, CustomValuePrinter) {
  UnprintableValue v = PB_TEST_CHOOSE_VALUE(
      (UnprintableValue{10, 20}, UnprintableValue{30, 40}),
      [tag = std::string("idx")](const UnprintableValue& val, size_t i) {
        return absl::StrCat(tag, i, "_", val.a + val.b);
      });
  int x = PB_TEST_CHOOSE_VALUE((1, 2), CustomIntPrinter);
  RecordedCustomPrintedValues().push_back(absl::StrCat(
      v.a, ":", v.b, ":", x, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, ExplicitValueNames) {
  int single = PB_TEST_CHOOSE_VALUE((5), {"five"});
  int single_prefixed = PB_TEST_CHOOSE_VALUE((6), "p", {"six"});
  UnprintableValue v =
      PB_TEST_CHOOSE_VALUE((UnprintableValue{10, 20}, UnprintableValue{30, 40}),
                           {"first", "second"});
  int x = PB_TEST_CHOOSE_VALUE((1, 2), "num", {"one-val", "one/val"});
  RecordedExplicitValueNames().push_back(absl::StrCat(
      single + single_prefixed, ":", v.a + v.b, ":", x, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, ValueNamePrefix) {
  int single = PB_TEST_CHOOSE_VALUE((7), "single");
  bool flag = PB_TEST_CHOOSE_VALUE((false, true), "Flag");
  int custom = PB_TEST_CHOOSE_VALUE((10, 20), "custom", [](int val, size_t i) {
    return absl::StrCat("v", static_cast<size_t>(val) + i);
  });
  RecordedPrefixedValues().push_back(absl::StrCat(
      single, ":", flag, ":", custom, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, NonStaticValues) {
  int i = 10;
  int* p = PB_TEST_CHOOSE_VALUE((nullptr, &i), {"null", "ptr"});
  UnprintableValue derived = PB_TEST_CHOOSE_VALUE(
      (UnprintableValue{i, 1}, UnprintableValue{i, 2}), "v");
  if (p != nullptr) {
    EXPECT_EQ(p, &i);
  }
  RecordedNonStaticValues().push_back(absl::StrCat(
      p == nullptr ? 0 : *p, ":", derived.a + derived.b, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, MultipleTypeDimensions) {
  using T1 = PB_TEST_CHOOSE_TYPE((int, double, float));
  using T2 = PB_TEST_CHOOSE_TYPE((char, bool));
  std::string type_pair =
      absl::StrCat(*GetTypeName<T1>(), "/", *GetTypeName<T2>());
  RecordedMultiTypes().push_back(type_pair);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_EQ(test_info->name(),
            absl::StrCat("MultipleTypeDimensions/", type_pair));
  EXPECT_EQ(test_info->value_param(), nullptr);
  EXPECT_THAT(test_info->type_param(), StrEq(type_pair));
}

PB_TEST(ProtobufStandaloneTest, TupleTypeInput) {
  using TupleAlias = std::tuple<int32_t, int64_t>;
  using T1 = PB_TEST_CHOOSE_TYPE(TupleAlias);
  using T2 = PB_TEST_CHOOSE_TYPE(std::tuple<char, bool>);
  using T3 = PB_TEST_CHOOSE_TYPE((TupleAlias));
  EXPECT_TRUE((std::is_same_v<T3, TupleAlias>));
  RecordedTupleTypes().push_back(absl::StrCat(
      *GetTypeName<T1>(), ":", *GetTypeName<T2>(), ":", *GetTypeName<T3>(), ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, TypesAndValuesCombined) {
  int a = PB_TEST_CHOOSE_VALUE((1, 2));
  absl::string_view b = PB_TEST_CHOOSE_VALUE(("x", "y"));
  using T1 = PB_TEST_CHOOSE_TYPE((int, double));
  using T2 = PB_TEST_CHOOSE_TYPE((char, bool));
  std::string value_part = absl::StrCat(a, "/", b);
  std::string type_part =
      absl::StrCat(*GetTypeName<T1>(), "/", *GetTypeName<T2>());
  RecordedTypesAndValues().push_back(absl::StrCat(value_part, "/", type_part));

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_EQ(test_info->name(), absl::StrCat("TypesAndValuesCombined/",
                                            value_part, "/", type_part));
  EXPECT_THAT(test_info->value_param(), StrEq(value_part));
  EXPECT_THAT(test_info->type_param(), StrEq(type_part));
}

PB_TEST(ProtobufStandaloneTest, TypeDependentValues) {
  using T = PB_TEST_CHOOSE_TYPE((int, double));
  T value = PB_TEST_CHOOSE_VALUE((T{0}, T{1}));
  RecordedTypeDependentValues().push_back(
      absl::StrCat(*GetTypeName<T>(), ":", value));
}

PB_TEST(ProtobufStandaloneTest, TypeDependentTypes) {
  using T = PB_TEST_CHOOSE_TYPE((int, double));
  using U = PB_TEST_CHOOSE_TYPE((std::vector<T>, std::deque<T>));
  EXPECT_TRUE((std::is_same_v<typename U::value_type, T>));
  U container = {T{1}, T{2}};
  EXPECT_EQ(container.size(), 2);
  absl::string_view container_type =
      std::is_same_v<U, std::vector<T>> ? "vector" : "deque";
  RecordedTypeDependentTypes().push_back(
      absl::StrCat(*GetTypeName<T>(), ":", container_type));
}

PB_TEST(ProtobufStandaloneTest, DisambiguatesDuplicateTypes) {
  using IntAlias = int;
  using T = PB_TEST_CHOOSE_TYPE((int, IntAlias, int));
  EXPECT_TRUE((std::is_same_v<T, int>));
  RecordedDupeTypeTestNames().emplace_back(
      ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

PB_TEST(ProtobufStandaloneTest, SanitizesTypeNames) {
  using T = PB_TEST_CHOOSE_TYPE((int, unsigned int, int*, ForeignEnum));
  EXPECT_GT(sizeof(T), 0);
  RecordedSanitizedTypeTestNames().emplace_back(
      ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

PB_TEST(ProtobufStandaloneTest, UnprintableTypesUseIndex) {
  using Tuple = std::tuple<std::array<int, 5>, int32_t>;
  using T1 =
      PB_TEST_CHOOSE_TYPE((std::array<int, 5>, int32_t, std::array<int, 10>));
  using T2 = PB_TEST_CHOOSE_TYPE(Tuple);
  EXPECT_GT(sizeof(T1) + sizeof(T2), 0);
  RecordedUnprintableTypeTestNames().emplace_back(
      ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

struct CustomTypePrinterImpl {
  template <typename T>
  static std::string Print(size_t index) {
    return absl::StrCat("idx", index, "-size", sizeof(T));
  }
};

PB_TEST(ProtobufStandaloneTest, CustomTypePrinter) {
  using T1 =
      PB_TEST_CHOOSE_TYPE((std::array<int, 5>, int64_t), CustomTypePrinterImpl);
  using T2 =
      PB_TEST_CHOOSE_TYPE(std::tuple<int8_t, int16_t>, CustomTypePrinterImpl);
  EXPECT_GT(sizeof(T1) + sizeof(T2), 0);
  RecordedCustomPrintedTypes().push_back(absl::StrCat(
      sizeof(T1), ":", sizeof(T2), ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

#if __cplusplus >= 202002L
PB_TEST(ProtobufStandaloneTest, ExplicitTypeNames) {
  using T1 =
      PB_TEST_CHOOSE_TYPE((std::array<int, 5>, int64_t), {"arr5", "i64"});
  using T2 =
      PB_TEST_CHOOSE_TYPE(std::tuple<int8_t, int16_t>, "tup", {"i8", "i16"});
  using T3 = PB_TEST_CHOOSE_TYPE((int32_t), "single", {"i32"});
  using T4 = PB_TEST_CHOOSE_TYPE((uint32_t), {"u32"});
  EXPECT_GT(sizeof(T1) + sizeof(T2) + sizeof(T3) + sizeof(T4), 0);
  RecordedExplicitTypeNames().push_back(absl::StrCat(
      sizeof(T1), ":", sizeof(T2), ":", sizeof(T3), ":", sizeof(T4), ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, TypeNamePrefix) {
  using T1 = PB_TEST_CHOOSE_TYPE((int32_t, int64_t), "first");
  using T2 = PB_TEST_CHOOSE_TYPE(std::tuple<char, bool>, "second");
  using T3 =
      PB_TEST_CHOOSE_TYPE((std::array<int, 5>), "third", CustomTypePrinterImpl);
  using T4 =
      PB_TEST_CHOOSE_TYPE(std::tuple<int16_t>, "fourth", CustomTypePrinterImpl);
  using T5 = PB_TEST_CHOOSE_TYPE((uint32_t), "fifth");
  EXPECT_GT(sizeof(T1) + sizeof(T2) + sizeof(T3) + sizeof(T4) + sizeof(T5), 0);
  RecordedPrefixedTypes().push_back(absl::StrCat(
      *GetTypeName<T1>(), ":", *GetTypeName<T2>(), ":", sizeof(T3), ":",
      sizeof(T4), ":", *GetTypeName<T5>(), ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}
#endif  // __cplusplus >= 202002L

PB_TEST(ProtobufStandaloneTest, ConditionalChooseFromType) {
  using T1 = PB_TEST_CHOOSE_TYPE((int32_t, double, char));
  if constexpr (std::is_same_v<T1, int32_t>) {
    using T2 = PB_TEST_CHOOSE_TYPE((bool, float));
    RecordedConditionalTypes().push_back(
        absl::StrCat(*GetTypeName<T1>(), ":", *GetTypeName<T2>()));
  } else if constexpr (std::is_same_v<T1, double>) {
    int v = PB_TEST_CHOOSE_VALUE((10, 20));
    RecordedConditionalTypes().push_back(
        absl::StrCat(*GetTypeName<T1>(), ":", v));
  } else {
    using T2 = PB_TEST_CHOOSE_TYPE((int16_t, uint16_t));
    RecordedConditionalTypes().push_back(
        absl::StrCat(*GetTypeName<T1>(), ":", *GetTypeName<T2>()));
  }
}

PB_TEST(ProtobufStandaloneTest, SameLineChooseType) {
  using PairType = std::pair<PB_TEST_CHOOSE_TYPE((int, double)),
                             PB_TEST_CHOOSE_TYPE((char, bool))>;
  RecordedSameLineTypes().push_back(
      absl::StrCat(*GetTypeName<typename PairType::first_type>(), "/",
                   *GetTypeName<typename PairType::second_type>()));
}

#define PB_TEST_DEFINE_TWO_ON_SAME_LINE()                \
  PB_TEST(ProtobufStandaloneTest, MacroExpandedFirst) {  \
    ++g_macro_first_runs;                                \
  }                                                      \
  PB_TEST(ProtobufStandaloneTest, MacroExpandedSecond) { \
    ++g_macro_second_runs;                               \
  }
PB_TEST_DEFINE_TWO_ON_SAME_LINE()
#undef PB_TEST_DEFINE_TWO_ON_SAME_LINE

PB_TEST(ProtobufStandaloneTest, RepeatedExecutionFails) {
  auto choose = [&](auto) { return PB_TEST_CHOOSE_VALUE((1, 2)); };
  int first = choose(1);
  EXPECT_TRUE(first == 1 || first == 2);
  EXPECT_NONFATAL_FAILURE(
      EXPECT_EQ(choose(1), first),
      "Attempted to call PB_TEST_CHOOSE_VALUE(...) twice in a single test run");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_DOUBLE_EQ(choose(1.0), first),
      "Attempted to call PB_TEST_CHOOSE_VALUE(...) twice in a single test run");
}

TEST(ProtobufMisuseTest, ChooseValueInRegularTestFails) {
  EXPECT_NONFATAL_FAILURE(EXPECT_EQ(PB_TEST_CHOOSE_VALUE((1, 2)), 1),
                          "Must use PB_TEST or PB_TEST_F");
}

class ProtobufRegularTestFFixture : public ::testing::Test {
 protected:
  int ChooseInFixture() { return PB_TEST_CHOOSE_VALUE((1, 2)); }
};

TEST_F(ProtobufRegularTestFFixture, ChooseValueInRegularTestFFails) {
  EXPECT_NONFATAL_FAILURE(EXPECT_EQ(PB_TEST_CHOOSE_VALUE((1, 2)), 1),
                          "Must use PB_TEST or PB_TEST_F");
  EXPECT_NONFATAL_FAILURE(EXPECT_EQ(ChooseInFixture(), 1),
                          "Must use PB_TEST or PB_TEST_F");
}

struct UnrelatedChooseHelper {
  int Choose() { return PB_TEST_CHOOSE_VALUE((1, 2)); }
};

PB_TEST(ProtobufMisuseTest, ChooseValueInUnrelatedClassFails) {
  UnrelatedChooseHelper helper;
  EXPECT_NONFATAL_FAILURE(
      EXPECT_EQ(helper.Choose(), 1),
      "PB_TEST_CHOOSE_VALUE(...) called from a class that is neither the "
      "current PB_TEST nor its direct fixture base class");
}

class ProtobufGrandparentFixture : public ::testing::Test {
 protected:
  int ChooseInGrandparent() { return PB_TEST_CHOOSE_VALUE((1, 2)); }
};

class ProtobufChildOfGrandparentFixture : public ProtobufGrandparentFixture {};

PB_TEST_F(ProtobufChildOfGrandparentFixture,
          ChooseValueInGrandparentFixtureFails) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_EQ(ChooseInGrandparent(), 1),
      "PB_TEST_CHOOSE_VALUE(...) called from a class that is neither the "
      "current PB_TEST nor its direct fixture base class");
}

TEST(ProtobufDeathTest, EmptyChooseValueDimensionTerminates) {
  EXPECT_DEATH(
      PartialTestRegistry::Get().AddValueDimension<ProtobufTestFixture>(
          static_cast<CounterT>(999999), nullptr,
          ChooseValueImpl(std::vector<int>{}), {"std::vector<int>{}"}),
      "PB_TEST_CHOOSE_VALUE requires at least one element");
  EXPECT_DEATH(
      PartialTestRegistry::Get().AddValueDimension<ProtobufTestFixture>(
          static_cast<CounterT>(999999), nullptr, 0, {"()"}),
      "PB_TEST_CHOOSE_VALUE requires at least one element");
}

class SampleTestWithParamFixture : public ::testing::TestWithParam<int> {};
class DerivedTestWithParamFixture : public SampleTestWithParamFixture {};
class CustomWithParamInterfaceFixture
    : public ::testing::Test,
      public ::testing::WithParamInterface<int> {};
template <typename T>
class SampleTemplateFixture : public ::testing::Test {};
struct NotATestFixture {};

static_assert(kIsValidProtobufTestFixture<::testing::Test>);
static_assert(kIsValidProtobufTestFixture<ProtobufTestFixture>);
static_assert(kIsValidProtobufTestFixture<SampleTemplateFixture<int>>);
static_assert(!kIsValidProtobufTestFixture<::testing::TestWithParam<int>>);
static_assert(!kIsValidProtobufTestFixture<SampleTestWithParamFixture>);
static_assert(!kIsValidProtobufTestFixture<DerivedTestWithParamFixture>);
static_assert(!kIsValidProtobufTestFixture<CustomWithParamInterfaceFixture>);
static_assert(!kIsValidProtobufTestFixture<NotATestFixture>);

std::vector<std::string>& RecordedParamGeneratorValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedConstexprValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedHeterogeneousConstexprValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

PB_TEST(ProtobufStandaloneTest, ParamGeneratorValues) {
  bool b = PB_TEST_CHOOSE_VALUE(testing::Bool(), "b");
  int r = PB_TEST_CHOOSE_VALUE(testing::Range(1, 3));
  int v = PB_TEST_CHOOSE_VALUE(
      std::vector<int>{10, 20}, "v",
      [](int val, size_t i) { return absl::StrCat("item", i, "_", val); });
  RecordedParamGeneratorValues().push_back(absl::StrCat(
      b, ":", r, ":", v, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, ChooseConstexprBasic) {
  static constexpr int kLocal = 1;
  constexpr int kVal = PB_TEST_CHOOSE_CONSTEXPR((kLocal, 2));
  int runtime_val = PB_TEST_CHOOSE_VALUE((5, 6), "rt");
  constexpr size_t kSeq =
      PB_TEST_CHOOSE_CONSTEXPR(std::index_sequence<10, 20>{}, "seq");
  constexpr int kArr =
      PB_TEST_CHOOSE_CONSTEXPR(std::array{100, 200}, "arr", {"a100", "a200"});
  std::array<int, kVal> arr{};
  EXPECT_EQ(arr.size(), kVal);
  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  RecordedConstexprValues().push_back(
      absl::StrCat(kVal, ":", runtime_val, ":", kSeq, ":", kArr, ":",
                   test_info->value_param(), ":", test_info->name()));
}

template <typename T, int N>
struct ConstexprTag {
  using type = T;
  static constexpr int kValue = N;
};

PB_TEST(ProtobufStandaloneTest, ChooseConstexprHeterogeneous) {
  auto tag = PB_TEST_CHOOSE_CONSTEXPR(
      (ConstexprTag<int32_t, 1>{}, ConstexprTag<int64_t, 2>{}));
  constexpr std::pair kPair =
      PB_TEST_CHOOSE_CONSTEXPR((std::pair{1, absl::string_view("a")},
                                std::pair{2, absl::string_view("b")}));
  auto custom = PB_TEST_CHOOSE_CONSTEXPR(
      std::tuple{int32_t{3}, int64_t{4}}, "c",
      [](auto v, size_t i) { return absl::StrCat("idx", i, "_", v); });
  using TagType = typename decltype(tag)::type;
  constexpr int kTagVal = decltype(tag)::kValue;
  constexpr int kPairFirst = kPair.first;
  RecordedHeterogeneousConstexprValues().push_back(absl::StrCat(
      sizeof(TagType), ":", kTagVal, ":", kPairFirst, ":", kPair.second, ":",
      sizeof(custom), ":", custom, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

class ProtobufValueInFixtureTest : public testing::Test {
 public:
  static std::vector<std::string>& runs() {
    static auto* const r = new std::vector<std::string>();
    return *r;
  }

 protected:
  void SetUp() override { value_ = PB_TEST_CHOOSE_VALUE((1, 2), "setup"); }

  template <typename T = int>
  T ChooseExtra() {
    return PB_TEST_CHOOSE_VALUE((T{10}, T{20}), "extra");
  }

  bool flag_ = PB_TEST_CHOOSE_VALUE(testing::Bool(), "ctor");
  int value_ = 0;
};

PB_TEST_F(ProtobufValueInFixtureTest, FromCtorSetUpAndHelper) {
  int extra = ChooseExtra();
  EXPECT_TRUE(value_ == 1 || value_ == 2);
  EXPECT_TRUE(extra == 10 || extra == 20);
  runs().push_back(absl::StrCat(
      flag_, ":", value_, ":", extra, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST_F(ProtobufValueInFixtureTest, CombinedWithBodyValueAndType) {
  using T = PB_TEST_CHOOSE_TYPE((int32_t, double));
  T extra = ChooseExtra<T>();
  int body = PB_TEST_CHOOSE_VALUE((100, 200), "body");
  runs().push_back(absl::StrCat(
      flag_, ":", value_, ":", extra, ":", body, ":", *GetTypeName<T>(), ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

class VerificationEnvironment : public ::testing::Environment {
 public:
  void TearDown() override {
    EXPECT_EQ(ProtobufTestFixture::set_up_suite_count_, 1);
    EXPECT_EQ(ProtobufTestFixture::tear_down_suite_count_, 1);
    EXPECT_THAT(
        ProtobufValueInFixtureTest::runs(),
        ElementsAre(
            "0:1:10:FromCtorSetUpAndHelper/setup_1/extra_10/no_ctor",
            "0:2:10:FromCtorSetUpAndHelper/setup_2/extra_10/no_ctor",
            "0:1:20:FromCtorSetUpAndHelper/setup_1/extra_20/no_ctor",
            "0:2:20:FromCtorSetUpAndHelper/setup_2/extra_20/no_ctor",
            "1:1:10:FromCtorSetUpAndHelper/setup_1/extra_10/ctor",
            "1:2:10:FromCtorSetUpAndHelper/setup_2/extra_10/ctor",
            "1:1:20:FromCtorSetUpAndHelper/setup_1/extra_20/ctor",
            "1:2:20:FromCtorSetUpAndHelper/setup_2/extra_20/ctor",
            "0:1:10:100:int32_t:CombinedWithBodyValueAndType/setup_1/extra_10/"
            "no_ctor/int32_t/body_100",
            "0:2:10:100:int32_t:CombinedWithBodyValueAndType/setup_2/extra_10/"
            "no_ctor/int32_t/body_100",
            "0:1:20:100:int32_t:CombinedWithBodyValueAndType/setup_1/extra_20/"
            "no_ctor/int32_t/body_100",
            "0:2:20:100:int32_t:CombinedWithBodyValueAndType/setup_2/extra_20/"
            "no_ctor/int32_t/body_100",
            "1:1:10:100:int32_t:CombinedWithBodyValueAndType/setup_1/extra_10/"
            "ctor/int32_t/body_100",
            "1:2:10:100:int32_t:CombinedWithBodyValueAndType/setup_2/extra_10/"
            "ctor/int32_t/body_100",
            "1:1:20:100:int32_t:CombinedWithBodyValueAndType/setup_1/extra_20/"
            "ctor/int32_t/body_100",
            "1:2:20:100:int32_t:CombinedWithBodyValueAndType/setup_2/extra_20/"
            "ctor/int32_t/body_100",
            "0:1:10:200:int32_t:CombinedWithBodyValueAndType/setup_1/extra_10/"
            "no_ctor/int32_t/body_200",
            "0:2:10:200:int32_t:CombinedWithBodyValueAndType/setup_2/extra_10/"
            "no_ctor/int32_t/body_200",
            "0:1:20:200:int32_t:CombinedWithBodyValueAndType/setup_1/extra_20/"
            "no_ctor/int32_t/body_200",
            "0:2:20:200:int32_t:CombinedWithBodyValueAndType/setup_2/extra_20/"
            "no_ctor/int32_t/body_200",
            "1:1:10:200:int32_t:CombinedWithBodyValueAndType/setup_1/extra_10/"
            "ctor/int32_t/body_200",
            "1:2:10:200:int32_t:CombinedWithBodyValueAndType/setup_2/extra_10/"
            "ctor/int32_t/body_200",
            "1:1:20:200:int32_t:CombinedWithBodyValueAndType/setup_1/extra_20/"
            "ctor/int32_t/body_200",
            "1:2:20:200:int32_t:CombinedWithBodyValueAndType/setup_2/extra_20/"
            "ctor/int32_t/body_200",
            "0:1:10:100:double:CombinedWithBodyValueAndType/setup_1/extra_10/"
            "no_ctor/double/body_100",
            "0:2:10:100:double:CombinedWithBodyValueAndType/setup_2/extra_10/"
            "no_ctor/double/body_100",
            "0:1:20:100:double:CombinedWithBodyValueAndType/setup_1/extra_20/"
            "no_ctor/double/body_100",
            "0:2:20:100:double:CombinedWithBodyValueAndType/setup_2/extra_20/"
            "no_ctor/double/body_100",
            "1:1:10:100:double:CombinedWithBodyValueAndType/setup_1/extra_10/"
            "ctor/double/body_100",
            "1:2:10:100:double:CombinedWithBodyValueAndType/setup_2/extra_10/"
            "ctor/double/body_100",
            "1:1:20:100:double:CombinedWithBodyValueAndType/setup_1/extra_20/"
            "ctor/double/body_100",
            "1:2:20:100:double:CombinedWithBodyValueAndType/setup_2/extra_20/"
            "ctor/double/body_100",
            "0:1:10:200:double:CombinedWithBodyValueAndType/setup_1/extra_10/"
            "no_ctor/double/body_200",
            "0:2:10:200:double:CombinedWithBodyValueAndType/setup_2/extra_10/"
            "no_ctor/double/body_200",
            "0:1:20:200:double:CombinedWithBodyValueAndType/setup_1/extra_20/"
            "no_ctor/double/body_200",
            "0:2:20:200:double:CombinedWithBodyValueAndType/setup_2/extra_20/"
            "no_ctor/double/body_200",
            "1:1:10:200:double:CombinedWithBodyValueAndType/setup_1/extra_10/"
            "ctor/double/body_200",
            "1:2:10:200:double:CombinedWithBodyValueAndType/setup_2/extra_10/"
            "ctor/double/body_200",
            "1:1:20:200:double:CombinedWithBodyValueAndType/setup_1/extra_20/"
            "ctor/double/body_200",
            "1:2:20:200:double:CombinedWithBodyValueAndType/setup_2/extra_20/"
            "ctor/double/body_200"));
    EXPECT_THAT(RecordedBools(), ElementsAre(false, true));
    EXPECT_THAT(RecordedEnums(),
                ElementsAre(FOREIGN_FOO, FOREIGN_BAR, FOREIGN_BAZ,
                            static_cast<ForeignEnum>(999),
                            static_cast<ForeignEnum>(-1)));
    EXPECT_THAT(RecordedSanitizedInputs(),
                ElementsAre(":-5", "///:-5", "__a//b--:-5", "foo-bar:-5",
                            "hello world!:-5", ":5", "///:5", "__a//b--:5",
                            "foo-bar:5", "hello world!:5", ":10", "///:10",
                            "__a//b--:10", "foo-bar:10", "hello world!:10"));
    EXPECT_THAT(RecordedDupeInputs(),
                ElementsAre("", "_", "a_b", "a-b", "a/b", "a b"));
    EXPECT_THAT(RecordedConditionalInputs(),
                ElementsAre(Pair(1, 10), Pair(1, 20)));
    EXPECT_THAT(RecordedSameLineSums(), ElementsAre(11, 12, 21, 22));
    EXPECT_THAT(
        RecordedUnprintableValues(),
        ElementsAre("30:3:UnprintableValuesUseIndex/UnprintableValue_10_20/"
                    "item_0",
                    "70:3:UnprintableValuesUseIndex/UnprintableValue_30_40/"
                    "item_0",
                    "30:7:UnprintableValuesUseIndex/UnprintableValue_10_20/"
                    "item_1",
                    "70:7:UnprintableValuesUseIndex/UnprintableValue_30_40/"
                    "item_1"));
    EXPECT_THAT(RecordedCustomPrintedValues(),
                ElementsAre("10:20:1:CustomValuePrinter/idx0_30/first_val",
                            "30:40:1:CustomValuePrinter/idx1_70/first_val",
                            "10:20:2:CustomValuePrinter/idx0_30/first_val_2",
                            "30:40:2:CustomValuePrinter/idx1_70/first_val_2"));
    EXPECT_THAT(
        RecordedExplicitValueNames(),
        ElementsAre("11:30:1:ExplicitValueNames/five/p_six/first/num_one_val",
                    "11:70:1:ExplicitValueNames/five/p_six/second/num_one_val",
                    "11:30:2:ExplicitValueNames/five/p_six/first/num_one_val_2",
                    "11:70:2:ExplicitValueNames/five/p_six/second/"
                    "num_one_val_2"));
    EXPECT_THAT(RecordedPrefixedValues(),
                ElementsAre("7:0:10:ValueNamePrefix/single_7/NoFlag/"
                            "custom_v10",
                            "7:1:10:ValueNamePrefix/single_7/Flag/"
                            "custom_v10",
                            "7:0:20:ValueNamePrefix/single_7/NoFlag/"
                            "custom_v21",
                            "7:1:20:ValueNamePrefix/single_7/Flag/"
                            "custom_v21"));
    EXPECT_THAT(
        RecordedNonStaticValues(),
        ElementsAre("0:11:NonStaticValues/null/v_UnprintableValue_i_1",
                    "10:11:NonStaticValues/ptr/v_UnprintableValue_i_1",
                    "0:12:NonStaticValues/null/v_UnprintableValue_i_2",
                    "10:12:NonStaticValues/ptr/v_UnprintableValue_i_2"));
    EXPECT_THAT(RecordedMultiTypes(),
                ElementsAre("int32_t/char", "int32_t/bool", "double/char",
                            "double/bool", "float/char", "float/bool"));
    EXPECT_THAT(
        RecordedTupleTypes(),
        ElementsAre("int32_t:char:std::tuple<int32_t, int64_t>:"
                    "TupleTypeInput/int32_t/char/tuple_int32_t_int64_t",
                    "int32_t:bool:std::tuple<int32_t, int64_t>:"
                    "TupleTypeInput/int32_t/bool/tuple_int32_t_int64_t",
                    "int64_t:char:std::tuple<int32_t, int64_t>:"
                    "TupleTypeInput/int64_t/char/tuple_int32_t_int64_t",
                    "int64_t:bool:std::tuple<int32_t, int64_t>:"
                    "TupleTypeInput/int64_t/bool/tuple_int32_t_int64_t"));
    EXPECT_THAT(
        RecordedTypesAndValues(),
        ElementsAre("1/x/int32_t/char", "2/x/int32_t/char", "1/y/int32_t/char",
                    "2/y/int32_t/char", "1/x/int32_t/bool", "2/x/int32_t/bool",
                    "1/y/int32_t/bool", "2/y/int32_t/bool", "1/x/double/char",
                    "2/x/double/char", "1/y/double/char", "2/y/double/char",
                    "1/x/double/bool", "2/x/double/bool", "1/y/double/bool",
                    "2/y/double/bool"));
    EXPECT_THAT(RecordedTypeDependentValues(),
                ElementsAre("int32_t:0", "int32_t:1", "double:0", "double:1"));
    EXPECT_THAT(RecordedTypeDependentTypes(),
                ElementsAre("int32_t:vector", "int32_t:deque", "double:vector",
                            "double:deque"));
    EXPECT_THAT(RecordedDupeTypeTestNames(),
                ElementsAre("DisambiguatesDuplicateTypes/int32_t",
                            "DisambiguatesDuplicateTypes/int32_t_2",
                            "DisambiguatesDuplicateTypes/int32_t_3"));
    EXPECT_THAT(
        RecordedSanitizedTypeTestNames(),
        ElementsAre("SanitizesTypeNames/int32_t", "SanitizesTypeNames/uint32_t",
                    "SanitizesTypeNames/int32_tPtr",
                    "SanitizesTypeNames/ForeignEnum"));
#if __cplusplus >= 202002L
    EXPECT_THAT(RecordedUnprintableTypeTestNames(),
                ElementsAre("UnprintableTypesUseIndex/array_int_5/0",
                            "UnprintableTypesUseIndex/array_int_5/int32_t",
                            "UnprintableTypesUseIndex/int32_t/0",
                            "UnprintableTypesUseIndex/int32_t/int32_t",
                            "UnprintableTypesUseIndex/array_int_10/0",
                            "UnprintableTypesUseIndex/array_int_10/int32_t"));
#else
    EXPECT_THAT(RecordedUnprintableTypeTestNames(),
                ElementsAre("UnprintableTypesUseIndex/0/0",
                            "UnprintableTypesUseIndex/0/int32_t",
                            "UnprintableTypesUseIndex/int32_t/0",
                            "UnprintableTypesUseIndex/int32_t/int32_t",
                            "UnprintableTypesUseIndex/2/0",
                            "UnprintableTypesUseIndex/2/int32_t"));
#endif  // __cplusplus >= 202002L
    EXPECT_THAT(RecordedCustomPrintedTypes(),
                ElementsAre("20:1:CustomTypePrinter/idx0_size20/idx0_size1",
                            "20:2:CustomTypePrinter/idx0_size20/idx1_size2",
                            "8:1:CustomTypePrinter/idx1_size8/idx0_size1",
                            "8:2:CustomTypePrinter/idx1_size8/idx1_size2"));
#if __cplusplus >= 202002L
    EXPECT_THAT(
        RecordedExplicitTypeNames(),
        ElementsAre("20:1:4:4:ExplicitTypeNames/arr5/tup_i8/single_i32/u32",
                    "20:2:4:4:ExplicitTypeNames/arr5/tup_i16/single_i32/u32",
                    "8:1:4:4:ExplicitTypeNames/i64/tup_i8/single_i32/u32",
                    "8:2:4:4:ExplicitTypeNames/i64/tup_i16/single_i32/u32"));
    EXPECT_THAT(
        RecordedPrefixedTypes(),
        ElementsAre(
            "int32_t:char:20:2:uint32_t:"
            "TypeNamePrefix/first_int32_t/second_char/third_idx0_size20/"
            "fourth_idx0_size2/fifth_uint32_t",
            "int32_t:bool:20:2:uint32_t:"
            "TypeNamePrefix/first_int32_t/second_bool/third_idx0_size20/"
            "fourth_idx0_size2/fifth_uint32_t",
            "int64_t:char:20:2:uint32_t:"
            "TypeNamePrefix/first_int64_t/second_char/third_idx0_size20/"
            "fourth_idx0_size2/fifth_uint32_t",
            "int64_t:bool:20:2:uint32_t:"
            "TypeNamePrefix/first_int64_t/second_bool/third_idx0_size20/"
            "fourth_idx0_size2/fifth_uint32_t"));
#endif  // __cplusplus >= 202002L
    EXPECT_THAT(RecordedConditionalTypes(),
                ElementsAre("int32_t:bool", "int32_t:float", "double:10",
                            "double:20", "char:int16_t", "char:uint16_t"));
    EXPECT_THAT(RecordedSameLineTypes(),
                ElementsAre("int32_t/char", "int32_t/bool", "double/char",
                            "double/bool"));
    EXPECT_EQ(g_macro_first_runs, 1);
    EXPECT_EQ(g_macro_second_runs, 1);
    EXPECT_THAT(RecordedParamGeneratorValues(),
                ElementsAre("0:1:10:ParamGeneratorValues/no_b/1/v_item0_10",
                            "1:1:10:ParamGeneratorValues/b/1/v_item0_10",
                            "0:2:10:ParamGeneratorValues/no_b/2/v_item0_10",
                            "1:2:10:ParamGeneratorValues/b/2/v_item0_10",
                            "0:1:20:ParamGeneratorValues/no_b/1/v_item1_20",
                            "1:1:20:ParamGeneratorValues/b/1/v_item1_20",
                            "0:2:20:ParamGeneratorValues/no_b/2/v_item1_20",
                            "1:2:20:ParamGeneratorValues/b/2/v_item1_20"));
    EXPECT_THAT(RecordedConstexprValues(),
                ElementsAre("1:5:10:100:1/rt_5/seq_10/arr_a100:"
                            "ChooseConstexprBasic/1/rt_5/seq_10/arr_a100",
                            "1:6:10:100:1/rt_6/seq_10/arr_a100:"
                            "ChooseConstexprBasic/1/rt_6/seq_10/arr_a100",
                            "1:5:10:200:1/rt_5/seq_10/arr_a200:"
                            "ChooseConstexprBasic/1/rt_5/seq_10/arr_a200",
                            "1:6:10:200:1/rt_6/seq_10/arr_a200:"
                            "ChooseConstexprBasic/1/rt_6/seq_10/arr_a200",
                            "1:5:20:100:1/rt_5/seq_20/arr_a100:"
                            "ChooseConstexprBasic/1/rt_5/seq_20/arr_a100",
                            "1:6:20:100:1/rt_6/seq_20/arr_a100:"
                            "ChooseConstexprBasic/1/rt_6/seq_20/arr_a100",
                            "1:5:20:200:1/rt_5/seq_20/arr_a200:"
                            "ChooseConstexprBasic/1/rt_5/seq_20/arr_a200",
                            "1:6:20:200:1/rt_6/seq_20/arr_a200:"
                            "ChooseConstexprBasic/1/rt_6/seq_20/arr_a200",
                            "2:5:10:100:2/rt_5/seq_10/arr_a100:"
                            "ChooseConstexprBasic/2/rt_5/seq_10/arr_a100",
                            "2:6:10:100:2/rt_6/seq_10/arr_a100:"
                            "ChooseConstexprBasic/2/rt_6/seq_10/arr_a100",
                            "2:5:10:200:2/rt_5/seq_10/arr_a200:"
                            "ChooseConstexprBasic/2/rt_5/seq_10/arr_a200",
                            "2:6:10:200:2/rt_6/seq_10/arr_a200:"
                            "ChooseConstexprBasic/2/rt_6/seq_10/arr_a200",
                            "2:5:20:100:2/rt_5/seq_20/arr_a100:"
                            "ChooseConstexprBasic/2/rt_5/seq_20/arr_a100",
                            "2:6:20:100:2/rt_6/seq_20/arr_a100:"
                            "ChooseConstexprBasic/2/rt_6/seq_20/arr_a100",
                            "2:5:20:200:2/rt_5/seq_20/arr_a200:"
                            "ChooseConstexprBasic/2/rt_5/seq_20/arr_a200",
                            "2:6:20:200:2/rt_6/seq_20/arr_a200:"
                            "ChooseConstexprBasic/2/rt_6/seq_20/arr_a200"));
    EXPECT_THAT(RecordedHeterogeneousConstexprValues(),
                ElementsAre("4:1:1:a:4:3:ChooseConstexprHeterogeneous/"
                            "ConstexprTag_int32_t_1/1_a/c_idx0_3",
                            "4:1:1:a:8:4:ChooseConstexprHeterogeneous/"
                            "ConstexprTag_int32_t_1/1_a/c_idx1_4",
                            "4:1:2:b:4:3:ChooseConstexprHeterogeneous/"
                            "ConstexprTag_int32_t_1/2_b/c_idx0_3",
                            "4:1:2:b:8:4:ChooseConstexprHeterogeneous/"
                            "ConstexprTag_int32_t_1/2_b/c_idx1_4",
                            "8:2:1:a:4:3:ChooseConstexprHeterogeneous/"
                            "ConstexprTag_int64_t_2/1_a/c_idx0_3",
                            "8:2:1:a:8:4:ChooseConstexprHeterogeneous/"
                            "ConstexprTag_int64_t_2/1_a/c_idx1_4",
                            "8:2:2:b:4:3:ChooseConstexprHeterogeneous/"
                            "ConstexprTag_int64_t_2/2_b/c_idx0_3",
                            "8:2:2:b:8:4:ChooseConstexprHeterogeneous/"
                            "ConstexprTag_int64_t_2/2_b/c_idx1_4"));
  }
};

[[maybe_unused]] const auto* const kVerificationEnv =
    ::testing::AddGlobalTestEnvironment(new VerificationEnvironment());

TEST(ProtobufTestRegistryTest, RegistersExpectedTestsWithoutDummySuite) {
  const auto* unit_test = ::testing::UnitTest::GetInstance();
  std::vector<std::string> suite_names;
  const ::testing::TestSuite* standalone_suite = nullptr;

  for (int i = 0; i < unit_test->total_test_suite_count(); ++i) {
    const auto* suite = unit_test->GetTestSuite(i);
    suite_names.emplace_back(suite->name());
    if (absl::string_view(suite->name()) == "ProtobufStandaloneTest") {
      standalone_suite = suite;
    }
  }

  EXPECT_THAT(
      suite_names,
      UnorderedElementsAre("ProtobufTestFixture", "ProtobufValueInFixtureTest",
                           "ProtobufStandaloneTest", "ProtobufTestRegistryTest",
                           "ProtobufDeathTest", "ProtobufMisuseTest",
                           "ProtobufRegularTestFFixture",
                           "ProtobufChildOfGrandparentFixture"));

  ASSERT_NE(standalone_suite, nullptr);
  std::vector<std::string> test_names;
  for (int i = 0; i < standalone_suite->total_test_count(); ++i) {
    test_names.emplace_back(standalone_suite->GetTestInfo(i)->name());
  }
  std::vector<std::string> expected_test_names = {
      "ThreeDimensions/1/a/100",
      "ThreeDimensions/2/a/100",
      "ThreeDimensions/1/b/100",
      "ThreeDimensions/2/b/100",
      "ThreeDimensions/1/a/200",
      "ThreeDimensions/2/a/200",
      "ThreeDimensions/1/b/200",
      "ThreeDimensions/2/b/200",
      "BoolValues/false",
      "BoolValues/true",
      "ProtoEnumValues/FOREIGN_FOO",
      "ProtoEnumValues/FOREIGN_BAR",
      "ProtoEnumValues/FOREIGN_BAZ",
      "ProtoEnumValues/999",
      "ProtoEnumValues/neg_1",
      "SanitizesNames/_/neg_5",
      "SanitizesNames/_2/neg_5",
      "SanitizesNames/a_b/neg_5",
      "SanitizesNames/foo_bar/neg_5",
      "SanitizesNames/hello_world/neg_5",
      "SanitizesNames/_/5",
      "SanitizesNames/_2/5",
      "SanitizesNames/a_b/5",
      "SanitizesNames/foo_bar/5",
      "SanitizesNames/hello_world/5",
      "SanitizesNames/_/10",
      "SanitizesNames/_2/10",
      "SanitizesNames/a_b/10",
      "SanitizesNames/foo_bar/10",
      "SanitizesNames/hello_world/10",
      "DisambiguatesDuplicateNames/_",
      "DisambiguatesDuplicateNames/_2",
      "DisambiguatesDuplicateNames/a_b",
      "DisambiguatesDuplicateNames/a_b_2",
      "DisambiguatesDuplicateNames/a_b_3",
      "DisambiguatesDuplicateNames/a_b_4",
      "ConditionalChooseValue/1/10",
      "ConditionalChooseValue/2/10",
      "ConditionalChooseValue/1/20",
      "ConditionalChooseValue/2/20",
      "SameLineChooseValue/1/10",
      "SameLineChooseValue/2/10",
      "SameLineChooseValue/1/20",
      "SameLineChooseValue/2/20",
      "UnprintableValuesUseIndex/UnprintableValue_10_20/item_0",
      "UnprintableValuesUseIndex/UnprintableValue_30_40/item_0",
      "UnprintableValuesUseIndex/UnprintableValue_10_20/item_1",
      "UnprintableValuesUseIndex/UnprintableValue_30_40/item_1",
      "CustomValuePrinter/idx0_30/first_val",
      "CustomValuePrinter/idx1_70/first_val",
      "CustomValuePrinter/idx0_30/first_val_2",
      "CustomValuePrinter/idx1_70/first_val_2",
      "ExplicitValueNames/five/p_six/first/num_one_val",
      "ExplicitValueNames/five/p_six/second/num_one_val",
      "ExplicitValueNames/five/p_six/first/num_one_val_2",
      "ExplicitValueNames/five/p_six/second/num_one_val_2",
      "ValueNamePrefix/single_7/NoFlag/custom_v10",
      "ValueNamePrefix/single_7/Flag/custom_v10",
      "ValueNamePrefix/single_7/NoFlag/custom_v21",
      "ValueNamePrefix/single_7/Flag/custom_v21",
      "NonStaticValues/null/v_UnprintableValue_i_1",
      "NonStaticValues/ptr/v_UnprintableValue_i_1",
      "NonStaticValues/null/v_UnprintableValue_i_2",
      "NonStaticValues/ptr/v_UnprintableValue_i_2",
      "MultipleTypeDimensions/int32_t/char",
      "MultipleTypeDimensions/int32_t/bool",
      "MultipleTypeDimensions/double/char",
      "MultipleTypeDimensions/double/bool",
      "MultipleTypeDimensions/float/char",
      "MultipleTypeDimensions/float/bool",
      "TupleTypeInput/int32_t/char/tuple_int32_t_int64_t",
      "TupleTypeInput/int32_t/bool/tuple_int32_t_int64_t",
      "TupleTypeInput/int64_t/char/tuple_int32_t_int64_t",
      "TupleTypeInput/int64_t/bool/tuple_int32_t_int64_t",
      "TypesAndValuesCombined/1/x/int32_t/char",
      "TypesAndValuesCombined/2/x/int32_t/char",
      "TypesAndValuesCombined/1/y/int32_t/char",
      "TypesAndValuesCombined/2/y/int32_t/char",
      "TypesAndValuesCombined/1/x/int32_t/bool",
      "TypesAndValuesCombined/2/x/int32_t/bool",
      "TypesAndValuesCombined/1/y/int32_t/bool",
      "TypesAndValuesCombined/2/y/int32_t/bool",
      "TypesAndValuesCombined/1/x/double/char",
      "TypesAndValuesCombined/2/x/double/char",
      "TypesAndValuesCombined/1/y/double/char",
      "TypesAndValuesCombined/2/y/double/char",
      "TypesAndValuesCombined/1/x/double/bool",
      "TypesAndValuesCombined/2/x/double/bool",
      "TypesAndValuesCombined/1/y/double/bool",
      "TypesAndValuesCombined/2/y/double/bool",
      "TypeDependentValues/int32_t/0",
      "TypeDependentValues/int32_t/1",
      "TypeDependentValues/double/0",
      "TypeDependentValues/double/1",
      "TypeDependentTypes/int32_t/vector_int32_t",
      "TypeDependentTypes/int32_t/deque_int32_t",
      "TypeDependentTypes/double/vector_double",
      "TypeDependentTypes/double/deque_double",
      "DisambiguatesDuplicateTypes/int32_t",
      "DisambiguatesDuplicateTypes/int32_t_2",
      "DisambiguatesDuplicateTypes/int32_t_3",
      "SanitizesTypeNames/int32_t",
      "SanitizesTypeNames/uint32_t",
      "SanitizesTypeNames/int32_tPtr",
      "SanitizesTypeNames/ForeignEnum",
#if __cplusplus >= 202002L
      "UnprintableTypesUseIndex/array_int_5/0",
      "UnprintableTypesUseIndex/array_int_5/int32_t",
      "UnprintableTypesUseIndex/int32_t/0",
      "UnprintableTypesUseIndex/int32_t/int32_t",
      "UnprintableTypesUseIndex/array_int_10/0",
      "UnprintableTypesUseIndex/array_int_10/int32_t",
#else
      "UnprintableTypesUseIndex/0/0",
      "UnprintableTypesUseIndex/0/int32_t",
      "UnprintableTypesUseIndex/int32_t/0",
      "UnprintableTypesUseIndex/int32_t/int32_t",
      "UnprintableTypesUseIndex/2/0",
      "UnprintableTypesUseIndex/2/int32_t",
#endif  // __cplusplus >= 202002L
      "CustomTypePrinter/idx0_size20/idx0_size1",
      "CustomTypePrinter/idx0_size20/idx1_size2",
      "CustomTypePrinter/idx1_size8/idx0_size1",
      "CustomTypePrinter/idx1_size8/idx1_size2",
#if __cplusplus >= 202002L
      "ExplicitTypeNames/arr5/tup_i8/single_i32/u32",
      "ExplicitTypeNames/arr5/tup_i16/single_i32/u32",
      "ExplicitTypeNames/i64/tup_i8/single_i32/u32",
      "ExplicitTypeNames/i64/tup_i16/single_i32/u32",
      "TypeNamePrefix/first_int32_t/second_char/third_idx0_size20/"
      "fourth_idx0_size2/fifth_uint32_t",
      "TypeNamePrefix/first_int32_t/second_bool/third_idx0_size20/"
      "fourth_idx0_size2/fifth_uint32_t",
      "TypeNamePrefix/first_int64_t/second_char/third_idx0_size20/"
      "fourth_idx0_size2/fifth_uint32_t",
      "TypeNamePrefix/first_int64_t/second_bool/third_idx0_size20/"
      "fourth_idx0_size2/fifth_uint32_t",
#endif  // __cplusplus >= 202002L
      "ConditionalChooseFromType/int32_t/bool",
      "ConditionalChooseFromType/int32_t/float",
      "ConditionalChooseFromType/double/10",
      "ConditionalChooseFromType/double/20",
      "ConditionalChooseFromType/char/int16_t",
      "ConditionalChooseFromType/char/uint16_t",
      "SameLineChooseType/int32_t/char",
      "SameLineChooseType/int32_t/bool",
      "SameLineChooseType/double/char",
      "SameLineChooseType/double/bool",
      "MacroExpandedFirst",
      "MacroExpandedSecond",
      "RepeatedExecutionFails/1",
      "RepeatedExecutionFails/2",
      "ParamGeneratorValues/no_b/1/v_item0_10",
      "ParamGeneratorValues/b/1/v_item0_10",
      "ParamGeneratorValues/no_b/2/v_item0_10",
      "ParamGeneratorValues/b/2/v_item0_10",
      "ParamGeneratorValues/no_b/1/v_item1_20",
      "ParamGeneratorValues/b/1/v_item1_20",
      "ParamGeneratorValues/no_b/2/v_item1_20",
      "ParamGeneratorValues/b/2/v_item1_20",
      "ChooseConstexprBasic/1/rt_5/seq_10/arr_a100",
      "ChooseConstexprBasic/1/rt_6/seq_10/arr_a100",
      "ChooseConstexprBasic/1/rt_5/seq_10/arr_a200",
      "ChooseConstexprBasic/1/rt_6/seq_10/arr_a200",
      "ChooseConstexprBasic/1/rt_5/seq_20/arr_a100",
      "ChooseConstexprBasic/1/rt_6/seq_20/arr_a100",
      "ChooseConstexprBasic/1/rt_5/seq_20/arr_a200",
      "ChooseConstexprBasic/1/rt_6/seq_20/arr_a200",
      "ChooseConstexprBasic/2/rt_5/seq_10/arr_a100",
      "ChooseConstexprBasic/2/rt_6/seq_10/arr_a100",
      "ChooseConstexprBasic/2/rt_5/seq_10/arr_a200",
      "ChooseConstexprBasic/2/rt_6/seq_10/arr_a200",
      "ChooseConstexprBasic/2/rt_5/seq_20/arr_a100",
      "ChooseConstexprBasic/2/rt_6/seq_20/arr_a100",
      "ChooseConstexprBasic/2/rt_5/seq_20/arr_a200",
      "ChooseConstexprBasic/2/rt_6/seq_20/arr_a200",
      "ChooseConstexprHeterogeneous/ConstexprTag_int32_t_1/1_a/c_idx0_3",
      "ChooseConstexprHeterogeneous/ConstexprTag_int32_t_1/1_a/c_idx1_4",
      "ChooseConstexprHeterogeneous/ConstexprTag_int32_t_1/2_b/c_idx0_3",
      "ChooseConstexprHeterogeneous/ConstexprTag_int32_t_1/2_b/c_idx1_4",
      "ChooseConstexprHeterogeneous/ConstexprTag_int64_t_2/1_a/c_idx0_3",
      "ChooseConstexprHeterogeneous/ConstexprTag_int64_t_2/1_a/c_idx1_4",
      "ChooseConstexprHeterogeneous/ConstexprTag_int64_t_2/2_b/c_idx0_3",
      "ChooseConstexprHeterogeneous/ConstexprTag_int64_t_2/2_b/c_idx1_4",
  };
  EXPECT_THAT(test_names, ElementsAreArray(expected_test_names));
}

TEST(ProtobufTestRegistryTest, ParseNamesFromLiteral) {
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(a, b, c)", 3),
              ElementsAre("a", "b", "c"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("  ( a , b , )  ", 2),
              ElementsAre("a", "b"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  "(Foo{1, 2}, Bar(3, 4), arr[i, j])", 3),
              ElementsAre("Foo_1_2", "Bar_3_4", "arr_i_j"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  "(static_cast<Arena*>(nullptr), &arena)", 2),
              ElementsAre("nullptr", "arena"));
  EXPECT_THAT(
      PartialTestRegistry::ParseNamesFromLiteral("(nullptr, &arena)", 2),
      ElementsAre("nullptr", "arena"));
  EXPECT_THAT(
      PartialTestRegistry::ParseNamesFromLiteral(
          "(DstState::kEmpty, DstState::kPopulated, DstState::kCleared)", 3),
      ElementsAre("kEmpty", "kPopulated", "kCleared"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  R"((DstState::kEmpty, DstState::kPopulated), "dst")", 2),
              ElementsAre("dst_kEmpty", "dst_kPopulated"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  "(ns::Sub1::kFoo, ns::Sub2::kFoo)", 2),
              ElementsAre("Sub1_kFoo", "Sub2_kFoo"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  "(ns::Sub1::kFoo, ns::Sub2::kFoo, other::kBar)", 3),
              ElementsAre("Sub1_kFoo", "Sub2_kFoo", "kBar"));
  EXPECT_THAT(
      PartialTestRegistry::ParseNamesFromLiteral("(ns1::kFoo, ns2::kBar)", 2),
      ElementsAre("kFoo", "kBar"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  "(ns1::kFoo, ns2::kFoo, ns3::kBar)", 3),
              ElementsAre("ns1_kFoo", "ns2_kFoo", "kBar"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(kFoo, ns::kFoo)", 2),
              ElementsAre("kFoo", "ns_kFoo"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  "(a::b::Foo{1, 2}, a::b::Foo{3, 4})", 2),
              ElementsAre("Foo_1_2", "Foo_3_4"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(foo.a, foo.b)", 2),
              ElementsAre("a", "b"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(foo->a, foo->b)", 2),
              ElementsAre("a", "b"));
  EXPECT_THAT(
      PartialTestRegistry::ParseNamesFromLiteral("(&Foo::a, &Foo::b)", 2),
      ElementsAre("a", "b"));
  EXPECT_THAT(
      PartialTestRegistry::ParseNamesFromLiteral("(Foo{a.x}, Foo{a.y})", 2),
      ElementsAre("Foo_a_x", "Foo_a_y"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(1.5, 1.75)", 2),
              ElementsAre("1_5", "1_75"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  "(\"a,b\", \"c\\\"d,e\", ',', '(', ')')", 5),
              ElementsAre("a_b", "c_d_e", "_", "_2", "_3"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  R"((x + 1, y + 2), "my_prefix")", 2),
              ElementsAre("my_prefix_x_1", "my_prefix_y_2"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  R"((x + 1, y + 2), {"first", "second"})", 2),
              ElementsAre("first", "second"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  R"((x + 1, y + 2), "my_prefix", {"first", "second"})", 2),
              ElementsAre("my_prefix_first", "my_prefix_second"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  R"((MACRO_EXPANDING_TO_TWO), {"first", "second"})", 2),
              ElementsAre("first", "second"));
  EXPECT_THAT(
      PartialTestRegistry::ParseNamesFromLiteral("(MACRO_EXPANDING_TO_TWO)", 2),
      ElementsAre("0", "1"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(a < b, c > d)", 2),
              ElementsAre("a_b", "c_d"));
  EXPECT_THAT(
      PartialTestRegistry::ParseNamesFromLiteral(
          R"((std::pair<int, int>(a, 1), std::pair<int, int>(a, 2)), "pair")",
          2),
      ElementsAre("pair_pair_int_int_a_1", "pair_pair_int_int_a_2"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral(
                  R"((FieldInfo<TestAllTypes, "optional_string">()))", 1),
              ElementsAre("FieldInfo_TestAllTypes_optional_string"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("not_parens", 2),
              ElementsAre("0", "1"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(a, {b)", 2),
              ElementsAre("0", "1"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(a, [b)", 2),
              ElementsAre("0", "1"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(a, }b)", 2),
              ElementsAre("0", "1"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(a, ]b)", 2),
              ElementsAre("0", "1"));
  EXPECT_THAT(PartialTestRegistry::ParseNamesFromLiteral("(a, b", 2),
              ElementsAre("0", "1"));
  EXPECT_THAT(
      PartialTestRegistry::ParseNamesFromLiteral(R"((a, "unclosed))", 2),
      ElementsAre("0", "1"));
}

TEST(ProtobufTestRegistryTest, ChooseValueImplNoAmbiguityForDifferentN) {
  auto printer = [](int v, size_t i) { return absl::StrCat(v, "_", i); };

  // N = 1
  auto v1 = ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10)));
  EXPECT_EQ(v1.name_prefix, absl::nullopt);
  EXPECT_EQ(v1.printer(10, 0), "10");

  auto v1_prefix = ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10), "pfx"));
  EXPECT_EQ(v1_prefix.name_prefix, "pfx");
  EXPECT_EQ(v1_prefix.printer(10, 0), "10");

  auto v1_printer = ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10), printer));
  EXPECT_EQ(v1_printer.name_prefix, absl::nullopt);
  EXPECT_EQ(v1_printer.printer(10, 0), "10_0");

  auto v1_prefix_printer =
      ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10), "pfx", printer));
  EXPECT_EQ(v1_prefix_printer.name_prefix, "pfx");
  EXPECT_EQ(v1_prefix_printer.printer(10, 0), "10_0");

  auto v1_names = ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10), {"ten"}));
  EXPECT_EQ(v1_names.name_prefix, absl::nullopt);
  EXPECT_EQ(v1_names.printer(10, 0), "ten");

  auto v1_prefix_names =
      ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10), "pfx", {"ten"}));
  EXPECT_EQ(v1_prefix_names.name_prefix, "pfx");
  EXPECT_EQ(v1_prefix_names.printer(10, 0), "ten");

  // N = 2
  auto v2_prefix = ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10, 20), "pfx"));
  EXPECT_EQ(v2_prefix.name_prefix, "pfx");

  auto v2_names =
      ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10, 20), {"ten", "twenty"}));
  EXPECT_EQ(v2_names.name_prefix, absl::nullopt);
  EXPECT_EQ(v2_names.printer(20, 1), "twenty");

  auto v2_prefix_names = ChooseValueImpl(
      PB_TEST_CHOOSE_VALUE_ARGS_((10, 20), "pfx", {"ten", "twenty"}));
  EXPECT_EQ(v2_prefix_names.name_prefix, "pfx");
  EXPECT_EQ(v2_prefix_names.printer(20, 1), "twenty");

  // N = 3
  auto v3_prefix =
      ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((10, 20, 30), "pfx"));
  EXPECT_EQ(v3_prefix.name_prefix, "pfx");

  auto v3_names = ChooseValueImpl(
      PB_TEST_CHOOSE_VALUE_ARGS_((10, 20, 30), {"ten", "twenty", "thirty"}));
  EXPECT_EQ(v3_names.name_prefix, absl::nullopt);
  EXPECT_EQ(v3_names.printer(30, 2), "thirty");

  auto v3_prefix_names = ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_(
      (10, 20, 30), "pfx", {"ten", "twenty", "thirty"}));
  EXPECT_EQ(v3_prefix_names.name_prefix, "pfx");
  EXPECT_EQ(v3_prefix_names.printer(30, 2), "thirty");

  // Iterable (ParamGenerator, std::vector, etc.)
  auto vg = ChooseValueImpl(testing::Range(10, 30, 10));
  EXPECT_THAT(vg.values, ElementsAre(10, 20));
  EXPECT_EQ(vg.name_prefix, absl::nullopt);
  EXPECT_EQ(vg.printer(10, 0), "10");

  auto vg_prefix = ChooseValueImpl(std::vector<int>{10, 20}, "pfx");
  EXPECT_THAT(vg_prefix.values, ElementsAre(10, 20));
  EXPECT_EQ(vg_prefix.name_prefix, "pfx");
  EXPECT_EQ(vg_prefix.printer(10, 0), "10");

  auto vg_printer = ChooseValueImpl(testing::Range(10, 30, 10), printer);
  EXPECT_THAT(vg_printer.values, ElementsAre(10, 20));
  EXPECT_EQ(vg_printer.name_prefix, absl::nullopt);
  EXPECT_EQ(vg_printer.printer(10, 0), "10_0");

  auto vg_prefix_printer =
      ChooseValueImpl(std::vector<int>{10, 20}, "pfx", printer);
  EXPECT_THAT(vg_prefix_printer.values, ElementsAre(10, 20));
  EXPECT_EQ(vg_prefix_printer.name_prefix, "pfx");
  EXPECT_EQ(vg_prefix_printer.printer(10, 0), "10_0");

  enum UnscopedEnum { kUnscopedTen = 10, kUnscopedTwenty = 20 };
  enum class ScopedEnum { kScopedTen = 10, kScopedTwenty = 20 };

  auto unscoped_literal = ChooseValueImpl(
      PB_TEST_CHOOSE_VALUE_ARGS_((kUnscopedTen, kUnscopedTwenty)));
  absl::optional<std::vector<std::string>> unscoped_literal_names;
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                unscoped_literal.printer, kUnscopedTen, 0,
                unscoped_literal.values.size(),
                "(kUnscopedTen, kUnscopedTwenty)", unscoped_literal_names),
            "kUnscopedTen");
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                unscoped_literal.printer, kUnscopedTwenty, 1,
                unscoped_literal.values.size(),
                "(kUnscopedTen, kUnscopedTwenty)", unscoped_literal_names),
            "kUnscopedTwenty");

  static constexpr UnscopedEnum kUnscopedArr[] = {kUnscopedTen,
                                                  kUnscopedTwenty};
  auto unscoped_arr = ChooseValueImpl(kUnscopedArr);
  absl::optional<std::vector<std::string>> unscoped_arr_names;
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                unscoped_arr.printer, kUnscopedTen, 0,
                unscoped_arr.values.size(), "kUnscopedArr", unscoped_arr_names),
            "10");
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                unscoped_arr.printer, kUnscopedTwenty, 1,
                unscoped_arr.values.size(), "kUnscopedArr", unscoped_arr_names),
            "20");

  static constexpr ScopedEnum kScopedArr[] = {ScopedEnum::kScopedTen,
                                              ScopedEnum::kScopedTwenty};
  auto scoped_arr = ChooseValueImpl(kScopedArr);
  absl::optional<std::vector<std::string>> scoped_arr_names;
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                scoped_arr.printer, ScopedEnum::kScopedTen, 0,
                scoped_arr.values.size(), "kScopedArr", scoped_arr_names),
            "10");
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                scoped_arr.printer, ScopedEnum::kScopedTwenty, 1,
                scoped_arr.values.size(), "kScopedArr", scoped_arr_names),
            "20");

  DefaultValuePrinter default_printer;
  EXPECT_EQ(default_printer(-1, 0), "neg_1");
  EXPECT_EQ(default_printer(std::numeric_limits<int8_t>::min(), 0), "neg_128");
  EXPECT_EQ(default_printer(std::numeric_limits<int16_t>::min(), 0),
            "neg_32768");
  EXPECT_EQ(default_printer(std::numeric_limits<int32_t>::min(), 0),
            "neg_2147483648");
  // "neg_9223372036854775808" is 23 chars (> kMaxPrintedLength = 20).
  EXPECT_EQ(default_printer(std::numeric_limits<int64_t>::min(), 0),
            absl::nullopt);
  EXPECT_EQ(default_printer(static_cast<char>(-5), 0),
            std::is_signed_v<char>
                ? "neg_5"
                : absl::StrCat(static_cast<unsigned char>(-5)));
  EXPECT_EQ(default_printer(-1.5, 0), "neg_1.5");
  EXPECT_EQ(default_printer(static_cast<proto2_unittest::ForeignEnum>(-7), 0),
            "neg_7");
  EXPECT_EQ(default_printer(
                std::make_tuple(std::numeric_limits<int32_t>::min(), -1), 0),
            "neg_2147483648_neg_1");

  proto2_unittest::ForeignMessage msg_a;
  msg_a.set_c(10);
  proto2_unittest::ForeignMessage msg_b;
  msg_b.set_c(20);
  EXPECT_EQ(default_printer(msg_a, 0), absl::nullopt);
  EXPECT_EQ(default_printer(std::make_tuple(msg_a, 1), 0), absl::nullopt);

  auto proto_literal =
      ChooseValueImpl(PB_TEST_CHOOSE_VALUE_ARGS_((msg_a, msg_b)));
  absl::optional<std::vector<std::string>> proto_literal_names;
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                proto_literal.printer, msg_a, 0, proto_literal.values.size(),
                "(msg_a, msg_b)", proto_literal_names),
            "msg_a");
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                proto_literal.printer, msg_b, 1, proto_literal.values.size(),
                "(msg_a, msg_b)", proto_literal_names),
            "msg_b");

  const std::string exact_max_str(DefaultValuePrinter::kMaxPrintedLength, 'a');
  const std::string too_long_str_a(DefaultValuePrinter::kMaxPrintedLength + 1,
                                   'a');
  const std::string too_long_str_b(DefaultValuePrinter::kMaxPrintedLength + 1,
                                   'b');
  EXPECT_EQ(default_printer(exact_max_str, 0), exact_max_str);
  EXPECT_EQ(default_printer(too_long_str_a, 0), absl::nullopt);
  EXPECT_EQ(default_printer(std::make_tuple(exact_max_str, "b"), 0),
            absl::nullopt);

  auto long_str_literal = ChooseValueImpl(
      PB_TEST_CHOOSE_VALUE_ARGS_((too_long_str_a, too_long_str_b)));
  absl::optional<std::vector<std::string>> long_str_literal_names;
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                long_str_literal.printer, too_long_str_a, 0,
                long_str_literal.values.size(),
                "(too_long_str_a, too_long_str_b)", long_str_literal_names),
            "too_long_str_a");
  EXPECT_EQ(PartialTestRegistry::GetValueElementName(
                long_str_literal.printer, too_long_str_b, 1,
                long_str_literal.values.size(),
                "(too_long_str_a, too_long_str_b)", long_str_literal_names),
            "too_long_str_b");
}

}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
