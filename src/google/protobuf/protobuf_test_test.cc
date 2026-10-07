// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/protobuf_test.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/unittest.pb.h"

namespace google::protobuf::internal {
namespace {

using ::proto2_unittest::FOREIGN_BAR;
using ::proto2_unittest::FOREIGN_BAZ;
using ::proto2_unittest::FOREIGN_FOO;
using ::proto2_unittest::ForeignEnum;
using ::testing::ElementsAre;
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
    EXPECT_EQ(set_up_count_, 1 + 1 + 3 + 6 + 3 + 4);
    EXPECT_EQ(tear_down_count_, 1 + 1 + 3 + 6 + 3 + 4);
    EXPECT_THAT(single_dim_values_, ElementsAre(10, 20, 30));
    EXPECT_THAT(multi_dim_values_,
                ElementsAre(Pair(1, "foo"), Pair(2, "foo"), Pair(1, "bar"),
                            Pair(2, "bar"), Pair(1, "baz"), Pair(2, "baz")));
    EXPECT_THAT(single_type_names_, ElementsAre("int32_t", "double", "bool"));
    EXPECT_THAT(type_and_value_runs_,
                ElementsAre(Pair(10, "int32_t"), Pair(20, "int32_t"),
                            Pair(10, "double"), Pair(20, "double")));
    ++tear_down_suite_count_;
  }

  static inline int set_up_suite_count_ = 0;
  static inline int tear_down_suite_count_ = 0;
  static inline int set_up_count_ = 0;
  static inline int tear_down_count_ = 0;
  static inline int regular_test_f_runs_ = 0;
  static inline int no_param_runs_ = 0;
  static inline std::vector<int> single_dim_values_;
  static inline std::vector<std::pair<int, std::string>> multi_dim_values_;
  static inline std::vector<std::string> single_type_names_;
  static inline std::vector<std::pair<int, std::string>> type_and_value_runs_;

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
  int v = PB_TEST_CHOOSE_VALUE({10, 20, 30});
  single_dim_values_.push_back(v);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(), absl::StrCat("SingleDimension/", v));
  EXPECT_THAT(test_info->value_param(), StrEq(absl::StrCat(v)));
  EXPECT_EQ(test_info->type_param(), nullptr);
}

PB_TEST_F(ProtobufTestFixture, MultiDimension) {
  EXPECT_EQ(fixture_state_, 42);
  int a = PB_TEST_CHOOSE_VALUE({1, 2});
  absl::string_view b = PB_TEST_CHOOSE_VALUE({"foo", "bar", "baz"});
  multi_dim_values_.emplace_back(a, std::string(b));

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
  single_type_names_.push_back(type_name);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(), absl::StrCat("SingleType/", type_name));
  EXPECT_EQ(test_info->value_param(), nullptr);
  EXPECT_THAT(test_info->type_param(), StrEq(type_name));
}

PB_TEST_F(ProtobufTestFixture, TypeAndValue) {
  EXPECT_EQ(fixture_state_, 42);
  int v = PB_TEST_CHOOSE_VALUE({10, 20});
  using T = PB_TEST_CHOOSE_TYPE((int, double));
  std::string type_name = *GetTypeName<T>();
  type_and_value_runs_.emplace_back(v, type_name);

  const auto* test_info =
      ::testing::UnitTest::GetInstance()->current_test_info();
  EXPECT_STREQ(test_info->test_suite_name(), "ProtobufTestFixture");
  EXPECT_EQ(test_info->name(),
            absl::StrCat("TypeAndValue/", v, "/", type_name));
  EXPECT_THAT(test_info->value_param(), StrEq(absl::StrCat(v)));
  EXPECT_THAT(test_info->type_param(), StrEq(type_name));
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

std::vector<std::string>& RecordedConstRefStrings() {
  static auto* v = new std::vector<std::string>();
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

std::vector<std::string>& RecordedPrefixedValues() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

std::vector<std::string>& RecordedCustomPrintedTypes() {
  static auto* v = new std::vector<std::string>();
  return *v;
}

#if __cpp_nontype_template_args >= 201911L || \
    (defined(__clang__) && __cplusplus >= 202002L)
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
  int x = PB_TEST_CHOOSE_VALUE({1, 2});
  absl::string_view y = PB_TEST_CHOOSE_VALUE({"a", "b"});
  int z = PB_TEST_CHOOSE_VALUE({100, 200});
  EXPECT_TRUE(x == 1 || x == 2);
  EXPECT_TRUE(y == "a" || y == "b");
  EXPECT_TRUE(z == 100 || z == 200);
  EXPECT_EQ(::testing::UnitTest::GetInstance()->current_test_info()->name(),
            absl::StrCat("ThreeDimensions/", x, "/", y, "/", z));
}

PB_TEST(ProtobufStandaloneTest, BoolValues) {
  const bool& b = PB_TEST_CHOOSE_VALUE({false, true});
  RecordedBools().push_back(b);
  EXPECT_EQ(::testing::UnitTest::GetInstance()->current_test_info()->name(),
            absl::StrCat("BoolValues/", b ? "true" : "false"));
}

PB_TEST(ProtobufStandaloneTest, ProtoEnumValues) {
  ForeignEnum e = PB_TEST_CHOOSE_VALUE({FOREIGN_FOO, FOREIGN_BAR, FOREIGN_BAZ,
                                        static_cast<ForeignEnum>(999),
                                        static_cast<ForeignEnum>(-1)});
  RecordedEnums().push_back(e);
}

PB_TEST(ProtobufStandaloneTest, SanitizesNames) {
  absl::string_view s =
      PB_TEST_CHOOSE_VALUE({"", "///", "__a//b--", "foo-bar", "hello world!"});
  int n = PB_TEST_CHOOSE_VALUE({-5, 5, 10});
  RecordedSanitizedInputs().push_back(absl::StrCat(s, ":", n));
}

PB_TEST(ProtobufStandaloneTest, DisambiguatesDuplicateNames) {
  absl::string_view s =
      PB_TEST_CHOOSE_VALUE({"", "_", "a_b", "a-b", "a/b", "a b"});
  RecordedDupeInputs().emplace_back(s);
}

PB_TEST(ProtobufStandaloneTest, ConditionalChooseValue) {
  int a = PB_TEST_CHOOSE_VALUE({1, 2});
  int b = 0;
  if (a == 1) {
    b = PB_TEST_CHOOSE_VALUE({10, 20});
  }
  RecordedConditionalInputs().emplace_back(a, b);
}

PB_TEST(ProtobufStandaloneTest, ReturnsConstReference) {
  decltype(auto) s =
      PB_TEST_CHOOSE_VALUE({std::string("hello"), std::string("world")});
  EXPECT_TRUE((std::is_same_v<decltype(s), const std::string&>));
  absl::string_view sv = s;
  RecordedConstRefStrings().emplace_back(sv);
}

PB_TEST(ProtobufStandaloneTest, SameLineChooseValue) {
  int v = PB_TEST_CHOOSE_VALUE({1, 2}) + PB_TEST_CHOOSE_VALUE({10, 20});
  RecordedSameLineSums().push_back(v);
}

struct UnprintableValue {
  int a;
  int b;
};

PB_TEST(ProtobufStandaloneTest, CustomValuePrinter) {
  UnprintableValue v =
      PB_TEST_CHOOSE_VALUE({UnprintableValue{10, 20}, UnprintableValue{30, 40}},
                           [](const UnprintableValue& val, size_t i) {
                             return absl::StrCat("idx", i, "_", val.a + val.b);
                           });
  int x = PB_TEST_CHOOSE_VALUE({1, 2}, [](int val, size_t) {
    return val == 1 ? "first-val" : "first/val";
  });
  RecordedCustomPrintedValues().push_back(absl::StrCat(
      v.a, ":", v.b, ":", x, ":",
      ::testing::UnitTest::GetInstance()->current_test_info()->name()));
}

PB_TEST(ProtobufStandaloneTest, ValueNamePrefix) {
  bool flag = PB_TEST_CHOOSE_VALUE({false, true}, "flag");
  int custom = PB_TEST_CHOOSE_VALUE({10, 20}, "custom", [](int val, size_t i) {
    return absl::StrCat("v", val + i);
  });
  RecordedPrefixedValues().push_back(absl::StrCat(
      flag, ":", custom, ":",
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
  int a = PB_TEST_CHOOSE_VALUE({1, 2});
  absl::string_view b = PB_TEST_CHOOSE_VALUE({"x", "y"});
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
  T value = PB_TEST_CHOOSE_VALUE({T{0}, T{1}});
  RecordedTypeDependentValues().push_back(
      absl::StrCat(*GetTypeName<T>(), ":", value));
}

PB_TEST(ProtobufStandaloneTest, TypeDependentTypes) {
#if __cpp_nontype_template_args >= 201911L || \
    (defined(__clang__) && __cplusplus >= 202002L)
  using T = PB_TEST_CHOOSE_TYPE((int, double), "T");
#else
  using T = PB_TEST_CHOOSE_TYPE((int, double));
#endif
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
  using T =
      PB_TEST_CHOOSE_TYPE((std::array<int, 5>, int32_t, std::array<int, 10>));
  EXPECT_GT(sizeof(T), 0);
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

#if __cpp_nontype_template_args >= 201911L || \
    (defined(__clang__) && __cplusplus >= 202002L)
PB_TEST(ProtobufStandaloneTest, TypeNamePrefix) {
  using T1 = PB_TEST_CHOOSE_TYPE((int32_t, int64_t), "first");
  using T2 = PB_TEST_CHOOSE_TYPE(std::tuple<char, bool>, "second");
  using T3 =
      PB_TEST_CHOOSE_TYPE((std::array<int, 5>), "third", CustomTypePrinterImpl);
  using T4 =
      PB_TEST_CHOOSE_TYPE(std::tuple<int16_t>, "fourth", CustomTypePrinterImpl);
  EXPECT_GT(sizeof(T1) + sizeof(T2) + sizeof(T3) + sizeof(T4), 0);
  RecordedPrefixedTypes().push_back(absl::StrCat(
      *GetTypeName<T1>(), ":", *GetTypeName<T2>(), ":", sizeof(T3), ":",
      sizeof(T4), ":",
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
    int v = PB_TEST_CHOOSE_VALUE({10, 20});
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

PB_TEST(ProtobufStandaloneTest, RepeatedExecutionReturnsSameValue) {
  auto choose = [&] { return PB_TEST_CHOOSE_VALUE({1, 2}); };
  int first = choose();
  int second = choose();
  EXPECT_EQ(first, second);
}

class VerificationEnvironment : public ::testing::Environment {
 public:
  void TearDown() override {
    EXPECT_EQ(ProtobufTestFixture::set_up_suite_count_, 1);
    EXPECT_EQ(ProtobufTestFixture::tear_down_suite_count_, 1);
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
                ElementsAre(Pair(1, 10), Pair(2, 0), Pair(1, 20), Pair(2, 0)));
    EXPECT_THAT(RecordedConstRefStrings(), ElementsAre("hello", "world"));
    EXPECT_THAT(RecordedSameLineSums(), ElementsAre(11, 12, 21, 22));
    EXPECT_THAT(RecordedCustomPrintedValues(),
                ElementsAre("10:20:1:CustomValuePrinter/idx0_30/first_val",
                            "30:40:1:CustomValuePrinter/idx1_70/first_val",
                            "10:20:2:CustomValuePrinter/idx0_30/first_val_2",
                            "30:40:2:CustomValuePrinter/idx1_70/first_val_2"));
    EXPECT_THAT(RecordedPrefixedValues(),
                ElementsAre("0:10:ValueNamePrefix/flag_false/custom_v10",
                            "1:10:ValueNamePrefix/flag_true/custom_v10",
                            "0:20:ValueNamePrefix/flag_false/custom_v21",
                            "1:20:ValueNamePrefix/flag_true/custom_v21"));
    EXPECT_THAT(RecordedMultiTypes(),
                ElementsAre("int32_t/char", "double/char", "double/bool",
                            "float/char", "float/bool", "int32_t/bool"));
    EXPECT_THAT(
        RecordedTupleTypes(),
        ElementsAre("int32_t:char:std::tuple<int32_t, int64_t>:"
                    "TupleTypeInput/int32_t/char/std_tuple_int32_t_int64_t",
                    "int64_t:char:std::tuple<int32_t, int64_t>:"
                    "TupleTypeInput/int64_t/char/std_tuple_int32_t_int64_t",
                    "int64_t:bool:std::tuple<int32_t, int64_t>:"
                    "TupleTypeInput/int64_t/bool/std_tuple_int32_t_int64_t",
                    "int32_t:bool:std::tuple<int32_t, int64_t>:"
                    "TupleTypeInput/int32_t/bool/std_tuple_int32_t_int64_t"));
    EXPECT_THAT(
        RecordedTypesAndValues(),
        ElementsAre("1/x/int32_t/char", "2/x/int32_t/char", "1/y/int32_t/char",
                    "2/y/int32_t/char", "1/x/double/char", "2/x/double/char",
                    "1/y/double/char", "2/y/double/char", "1/x/double/bool",
                    "2/x/double/bool", "1/y/double/bool", "2/y/double/bool",
                    "1/x/int32_t/bool", "2/x/int32_t/bool", "1/y/int32_t/bool",
                    "2/y/int32_t/bool"));
    EXPECT_THAT(RecordedTypeDependentValues(),
                ElementsAre("int32_t:0", "int32_t:1", "double:0", "double:1"));
    EXPECT_THAT(RecordedTypeDependentTypes(),
                ElementsAre("int32_t:vector", "double:vector", "double:deque",
                            "int32_t:deque"));
    EXPECT_THAT(RecordedDupeTypeTestNames(),
                ElementsAre("DisambiguatesDuplicateTypes/int32_t",
                            "DisambiguatesDuplicateTypes/int32_t_2",
                            "DisambiguatesDuplicateTypes/int32_t_3"));
    EXPECT_THAT(
        RecordedSanitizedTypeTestNames(),
        ElementsAre("SanitizesTypeNames/int32_t", "SanitizesTypeNames/uint32_t",
                    "SanitizesTypeNames/int32_tPtr",
                    "SanitizesTypeNames/proto2_unittest_ForeignEnum"));
    EXPECT_THAT(RecordedUnprintableTypeTestNames(),
                ElementsAre("UnprintableTypesUseIndex/0",
                            "UnprintableTypesUseIndex/int32_t",
                            "UnprintableTypesUseIndex/2"));
    EXPECT_THAT(RecordedCustomPrintedTypes(),
                ElementsAre("20:1:CustomTypePrinter/idx0_size20/idx0_size1",
                            "8:1:CustomTypePrinter/idx1_size8/idx0_size1",
                            "8:2:CustomTypePrinter/idx1_size8/idx1_size2",
                            "20:2:CustomTypePrinter/idx0_size20/idx1_size2"));
#if __cpp_nontype_template_args >= 201911L || \
    (defined(__clang__) && __cplusplus >= 202002L)
    EXPECT_THAT(
        RecordedPrefixedTypes(),
        ElementsAre(
            "int32_t:char:20:2:"
            "TypeNamePrefix/first_int32_t/second_char/third_idx0_size20/"
            "fourth_idx0_size2",
            "int64_t:char:20:2:"
            "TypeNamePrefix/first_int64_t/second_char/third_idx0_size20/"
            "fourth_idx0_size2",
            "int64_t:bool:20:2:"
            "TypeNamePrefix/first_int64_t/second_bool/third_idx0_size20/"
            "fourth_idx0_size2",
            "int32_t:bool:20:2:"
            "TypeNamePrefix/first_int32_t/second_bool/third_idx0_size20/"
            "fourth_idx0_size2"));
#endif  // __cplusplus >= 202002L
    EXPECT_THAT(RecordedConditionalTypes(),
                ElementsAre("int32_t:bool", "double:10", "double:20",
                            "char:int16_t", "char:uint16_t", "int32_t:float"));
    EXPECT_THAT(RecordedSameLineTypes(),
                ElementsAre("int32_t/char", "double/char", "double/bool",
                            "int32_t/bool"));
    EXPECT_EQ(g_macro_first_runs, 1);
    EXPECT_EQ(g_macro_second_runs, 1);
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

  EXPECT_THAT(suite_names, UnorderedElementsAre("ProtobufTestFixture",
                                                "ProtobufStandaloneTest",
                                                "ProtobufTestRegistryTest"));

  ASSERT_NE(standalone_suite, nullptr);
  std::vector<std::string> test_names;
  for (int i = 0; i < standalone_suite->total_test_count(); ++i) {
    test_names.emplace_back(standalone_suite->GetTestInfo(i)->name());
  }
  EXPECT_THAT(
      test_names,
      ElementsAre(
          "ThreeDimensions/1/a/100", "ThreeDimensions/2/a/100",
          "ThreeDimensions/1/b/100", "ThreeDimensions/2/b/100",
          "ThreeDimensions/1/a/200", "ThreeDimensions/2/a/200",
          "ThreeDimensions/1/b/200", "ThreeDimensions/2/b/200",
          "BoolValues/false", "BoolValues/true", "ProtoEnumValues/FOREIGN_FOO",
          "ProtoEnumValues/FOREIGN_BAR", "ProtoEnumValues/FOREIGN_BAZ",
          "ProtoEnumValues/999", "ProtoEnumValues/1", "SanitizesNames/_/5",
          "SanitizesNames/_2/5", "SanitizesNames/a_b/5",
          "SanitizesNames/foo_bar/5", "SanitizesNames/hello_world/5",
          "SanitizesNames/_/5_2", "SanitizesNames/_2/5_2",
          "SanitizesNames/a_b/5_2", "SanitizesNames/foo_bar/5_2",
          "SanitizesNames/hello_world/5_2", "SanitizesNames/_/10",
          "SanitizesNames/_2/10", "SanitizesNames/a_b/10",
          "SanitizesNames/foo_bar/10", "SanitizesNames/hello_world/10",
          "DisambiguatesDuplicateNames/_", "DisambiguatesDuplicateNames/_2",
          "DisambiguatesDuplicateNames/a_b",
          "DisambiguatesDuplicateNames/a_b_2",
          "DisambiguatesDuplicateNames/a_b_3",
          "DisambiguatesDuplicateNames/a_b_4", "ConditionalChooseValue/1/10",
          "ConditionalChooseValue/2/10", "ConditionalChooseValue/1/20",
          "ConditionalChooseValue/2/20", "ReturnsConstReference/hello",
          "ReturnsConstReference/world", "SameLineChooseValue/1/10",
          "SameLineChooseValue/2/10", "SameLineChooseValue/1/20",
          "SameLineChooseValue/2/20", "CustomValuePrinter/idx0_30/first_val",
          "CustomValuePrinter/idx1_70/first_val",
          "CustomValuePrinter/idx0_30/first_val_2",
          "CustomValuePrinter/idx1_70/first_val_2",
          "ValueNamePrefix/flag_false/custom_v10",
          "ValueNamePrefix/flag_true/custom_v10",
          "ValueNamePrefix/flag_false/custom_v21",
          "ValueNamePrefix/flag_true/custom_v21",
          "MultipleTypeDimensions/int32_t/char",
          "MultipleTypeDimensions/double/char",
          "MultipleTypeDimensions/double/bool",
          "MultipleTypeDimensions/float/char",
          "MultipleTypeDimensions/float/bool",
          "MultipleTypeDimensions/int32_t/bool",
          "TupleTypeInput/int32_t/char/std_tuple_int32_t_int64_t",
          "TupleTypeInput/int64_t/char/std_tuple_int32_t_int64_t",
          "TupleTypeInput/int64_t/bool/std_tuple_int32_t_int64_t",
          "TupleTypeInput/int32_t/bool/std_tuple_int32_t_int64_t",
          "TypesAndValuesCombined/1/x/int32_t/char",
          "TypesAndValuesCombined/2/x/int32_t/char",
          "TypesAndValuesCombined/1/y/int32_t/char",
          "TypesAndValuesCombined/2/y/int32_t/char",
          "TypesAndValuesCombined/1/x/double/char",
          "TypesAndValuesCombined/2/x/double/char",
          "TypesAndValuesCombined/1/y/double/char",
          "TypesAndValuesCombined/2/y/double/char",
          "TypesAndValuesCombined/1/x/double/bool",
          "TypesAndValuesCombined/2/x/double/bool",
          "TypesAndValuesCombined/1/y/double/bool",
          "TypesAndValuesCombined/2/y/double/bool",
          "TypesAndValuesCombined/1/x/int32_t/bool",
          "TypesAndValuesCombined/2/x/int32_t/bool",
          "TypesAndValuesCombined/1/y/int32_t/bool",
          "TypesAndValuesCombined/2/y/int32_t/bool",
          "TypeDependentValues/0/int32_t", "TypeDependentValues/1/int32_t",
          "TypeDependentValues/0/double", "TypeDependentValues/1/double",
#if __cpp_nontype_template_args >= 201911L || \
    (defined(__clang__) && __cplusplus >= 202002L)
          "TypeDependentTypes/T_int32_t/std_vector_int32_t",
          "TypeDependentTypes/T_double/std_vector_double",
          "TypeDependentTypes/T_double/std_deque_double",
          "TypeDependentTypes/T_int32_t/std_deque_int32_t",
#else
          "TypeDependentTypes/int32_t/std_vector_int32_t",
          "TypeDependentTypes/double/std_vector_double",
          "TypeDependentTypes/double/std_deque_double",
          "TypeDependentTypes/int32_t/std_deque_int32_t",
#endif
          "DisambiguatesDuplicateTypes/int32_t",
          "DisambiguatesDuplicateTypes/int32_t_2",
          "DisambiguatesDuplicateTypes/int32_t_3", "SanitizesTypeNames/int32_t",
          "SanitizesTypeNames/uint32_t", "SanitizesTypeNames/int32_tPtr",
          "SanitizesTypeNames/proto2_unittest_ForeignEnum",
          "UnprintableTypesUseIndex/0", "UnprintableTypesUseIndex/int32_t",
          "UnprintableTypesUseIndex/2",
          "CustomTypePrinter/idx0_size20/idx0_size1",
          "CustomTypePrinter/idx1_size8/idx0_size1",
          "CustomTypePrinter/idx1_size8/idx1_size2",
          "CustomTypePrinter/idx0_size20/idx1_size2",
#if __cpp_nontype_template_args >= 201911L || \
    (defined(__clang__) && __cplusplus >= 202002L)
          "TypeNamePrefix/first_int32_t/second_char/third_idx0_size20/"
          "fourth_idx0_size2",
          "TypeNamePrefix/first_int64_t/second_char/third_idx0_size20/"
          "fourth_idx0_size2",
          "TypeNamePrefix/first_int64_t/second_bool/third_idx0_size20/"
          "fourth_idx0_size2",
          "TypeNamePrefix/first_int32_t/second_bool/third_idx0_size20/"
          "fourth_idx0_size2",
#endif  // __cplusplus >= 202002L
          "ConditionalChooseFromType/int32_t/bool",
          "ConditionalChooseFromType/10/double",
          "ConditionalChooseFromType/20/double",
          "ConditionalChooseFromType/char/int16_t",
          "ConditionalChooseFromType/char/uint16_t",
          "ConditionalChooseFromType/int32_t/float",
          "SameLineChooseType/int32_t/char", "SameLineChooseType/double/char",
          "SameLineChooseType/double/bool", "SameLineChooseType/int32_t/bool",
          "MacroExpandedFirst", "MacroExpandedSecond",
          "RepeatedExecutionReturnsSameValue/1",
          "RepeatedExecutionReturnsSameValue/2"));
}

}  // namespace
}  // namespace protobuf
}  // namespace google::internal
