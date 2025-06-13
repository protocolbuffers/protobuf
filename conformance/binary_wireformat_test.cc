#include "binary_wireformat.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "absl/strings/string_view.h"

namespace google {
namespace protobuf {
namespace conformance {
namespace {

TEST(WireTest, Constructor) {
  Wire wire("foo");

  EXPECT_EQ(wire.size(), 3);
  EXPECT_EQ(wire.data(), "foo");
}

TEST(WireTest, ConstructorConcat) {
  Wire wire("foo", "bar");

  EXPECT_EQ(wire.size(), 6);
  EXPECT_EQ(wire.data(), "foobar");
}

TEST(WireTest, CopyConstructor) {
  Wire wire("foo");
  Wire copy(wire);

  EXPECT_EQ(copy.data(), "foo");
}

TEST(WireTest, MoveConstructor) {
  Wire wire("foo");
  Wire move(std::move(wire));

  EXPECT_EQ(move.data(), "foo");
}

TEST(WireTest, CopyAssignement) {
  Wire wire("foo");
  Wire copy;
  copy = wire;

  EXPECT_EQ(copy.data(), "foo");
}

TEST(WireTest, MoveAssignement) {
  Wire wire("foo");
  Wire move;
  move = std::move(wire);

  EXPECT_EQ(move.data(), "foo");
}

TEST(WireTest, Comparison) {
  Wire wire("foo", absl::string_view("\0", 1), "bar");

  ASSERT_EQ(wire.size(), 7);
  EXPECT_TRUE(wire == Wire(absl::string_view("foo\0bar", 7)));
  EXPECT_FALSE(wire != Wire(absl::string_view("foo\0bar", 7)));
  EXPECT_FALSE(wire == Wire("foo"));
  EXPECT_TRUE(wire != Wire("foo"));
}

TEST(WireTest, Stringify) {
  Wire wire("\xa0\032");
  std::string str = absl::StrCat(wire, Wire("abc"));

  EXPECT_EQ(str, "\240\032abc");
}

TEST(WireTest, PrintTo) {
  Wire wire("\xa0\032abc");
  std::string str = testing::PrintToString(wire);

  EXPECT_EQ(str, "\\240\\032abc");
}

struct TagTestCase {
  std::string name;
  uint32_t fieldnum;
  WireType wire_type;
  std::string expected;
};

class TagTest : public testing::TestWithParam<TagTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    TagTest, TagTest,
    testing::ValuesIn(std::vector<TagTestCase>{
        {.name = "Varint",
         .fieldnum = 1,
         .wire_type = WireType::kVarint,
         .expected = "\010"},
        {.name = "Fixed32",
         .fieldnum = 2,
         .wire_type = WireType::kFixed32,
         .expected = "\025"},
        {.name = "Fixed64",
         .fieldnum = 3,
         .wire_type = WireType::kFixed64,
         .expected = "\031"},
        {.name = "LengthPrefixed",
         .fieldnum = 4,
         .wire_type = WireType::kLengthPrefixed,
         .expected = "\042"},
        {.name = "StartGroup",
         .fieldnum = 5,
         .wire_type = WireType::kStartGroup,
         .expected = "\053"},
        {.name = "EndGroup",
         .fieldnum = 6,
         .wire_type = WireType::kEndGroup,
         .expected = "\064"},
        {.name = "TwoByteTag",
         .fieldnum = 256,
         .wire_type = WireType::kStartGroup,
         .expected = "\203\020"},
        {.name = "ThreeByteTag",
         .fieldnum = 0xFFFF,
         .wire_type = WireType::kStartGroup,
         .expected = "\373\377\037"}}),
    [](const testing::TestParamInfo<TagTest::ParamType>& info) {
      return info.param.name;
    });

TEST_P(TagTest, Check) {
  auto& param = GetParam();
  EXPECT_EQ(Tag(param.fieldnum, param.wire_type), Wire(param.expected));
}

struct VarintTestCase {
  uint64_t value;
  std::string expected;
};

class VarintTest : public testing::TestWithParam<VarintTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    VarintTest, VarintTest,
    testing::ValuesIn(std::vector<VarintTestCase>{
        {0, std::string("\000", 1)},
        {1, "\001"},
        {127, "\177"},
        {128, "\200\001"},
        {128 * 128, "\200\200\001"},
        {128 * 128 * 128, "\200\200\200\001"}}),
    [](const testing::TestParamInfo<VarintTest::ParamType>& info) {
      return absl::StrCat(info.param.value);
    });

TEST_P(VarintTest, Check) {
  auto& param = GetParam();
  EXPECT_EQ(Varint(param.value), Wire(param.expected));
}

struct LongVarintTestCase {
  uint64_t value;
  int extra;
  std::string expected;
};

class LongVarintTest : public testing::TestWithParam<LongVarintTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    LongVarintTest, LongVarintTest,
    testing::ValuesIn(std::vector<LongVarintTestCase>{
        {0, 1, std::string("\000", 1)},
        {1, 1, std::string("\201\000", 2)},
        {1, 4, std::string("\201\200\200\200\000", 5)},
        {127, 1, std::string("\377\000", 2)},
        {128, 1, std::string("\200\201\000", 3)}}),
    [](const testing::TestParamInfo<LongVarintTest::ParamType>& info) {
      return absl::StrCat(info.param.value, "_", info.param.extra);
    });

TEST_P(LongVarintTest, Check) {
  auto& param = GetParam();
  EXPECT_EQ(LongVarint(param.value, param.extra), Wire(param.expected));
}

struct SInt32TestCase {
  int32_t value;
  std::string expected;
};

class SInt32Test : public testing::TestWithParam<SInt32TestCase> {};

INSTANTIATE_TEST_SUITE_P(
    SInt32Test, SInt32Test,
    testing::ValuesIn(std::vector<SInt32TestCase>{
        {0, std::string("\000", 1)},  //
        {1, "\002"},
        {-1, "\001"}}),
    [](const testing::TestParamInfo<SInt32Test::ParamType>& info) {
      return absl::StrReplaceAll(absl::StrCat(info.param.value),
                                 {{"-", "neg"}});
    });

TEST_P(SInt32Test, Check) {
  auto& param = GetParam();
  EXPECT_EQ(SInt32(param.value), Wire(param.expected));
}

struct SInt64TestCase {
  int64_t value;
  std::string expected;
};

class SInt64Test : public testing::TestWithParam<SInt64TestCase> {};

INSTANTIATE_TEST_SUITE_P(
    SInt64Test, SInt64Test,
    testing::ValuesIn(std::vector<SInt64TestCase>{
        {0, std::string("\000", 1)},  //
        {1, "\002"},
        {-1, "\001"}}),
    [](const testing::TestParamInfo<SInt64Test::ParamType>& info) {
      return absl::StrReplaceAll(absl::StrCat(info.param.value),
                                 {{"-", "neg"}});
    });

TEST_P(SInt64Test, Check) {
  auto& param = GetParam();
  EXPECT_EQ(SInt64(param.value), Wire(param.expected));
}

struct Fixed32TestCase {
  uint32_t value;
  std::string expected;
};

class Fixed32Test : public testing::TestWithParam<Fixed32TestCase> {};

INSTANTIATE_TEST_SUITE_P(
    Fixed32Test, Fixed32Test,
    testing::ValuesIn(std::vector<Fixed32TestCase>{
        {0, std::string(4, '\0')},
        {128, std::string("\200\0\0\0", 4)},
        {0x80000000U, std::string("\0\0\0\200", 4)},
        {0xFEDCBA98U, "\x98\xba\xdc\xfe"},
        {0xFFFFFFFFU, "\xff\xff\xff\xff"},
    }),
    [](const testing::TestParamInfo<Fixed32Test::ParamType>& info) {
      return absl::StrCat(info.param.value);
    });

TEST_P(Fixed32Test, Check) {
  auto& param = GetParam();
  EXPECT_EQ(Fixed32(param.value), Wire(param.expected));
}

struct Fixed64TestCase {
  uint64_t value;
  std::string expected;
};

class Fixed64Test : public testing::TestWithParam<Fixed64TestCase> {};

INSTANTIATE_TEST_SUITE_P(
    Fixed64Test, Fixed64Test,
    testing::ValuesIn(std::vector<Fixed64TestCase>{
        {0, std::string(8, '\0')},
        {128, std::string("\200\0\0\0\0\0\0\0", 8)},
        {0x8000000000000000UL, std::string("\0\0\0\0\0\0\0\200", 8)},
        {0xFEDCBA9876543210UL, "\x10\x32\x54\x76\x98\xba\xdc\xfe"},
        {0xFFFFFFFFFFFFFFFFUL, "\xff\xff\xff\xff\xff\xff\xff\xff"},
    }),
    [](const testing::TestParamInfo<Fixed64Test::ParamType>& info) {
      return absl::StrCat(info.param.value);
    });

TEST_P(Fixed64Test, Check) {
  auto& param = GetParam();
  EXPECT_EQ(Fixed64(param.value), Wire(param.expected));
}

struct FloatTestCase {
  float value;
  std::string expected;
};

class FloatTest : public testing::TestWithParam<FloatTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    FloatTest, FloatTest,
    testing::ValuesIn(std::vector<FloatTestCase>{
        {0, std::string(4, '\0')},
        {-1, std::string("\0\0\200\277", 4)},
        {1, std::string("\0\0\200\077", 4)},
        {1.5, std::string("\0\0\300\077", 4)},
        {1.123456, "\150\315\217\077"},
    }),
    [](const testing::TestParamInfo<FloatTest::ParamType>& info) {
      return absl::StrReplaceAll(absl::StrCat(info.param.value),
                                 {{".", "_"}, {"-", "neg"}});
    });

TEST_P(FloatTest, Check) {
  auto& param = GetParam();
  EXPECT_EQ(Float(param.value), Wire(param.expected));
}

struct DoubleTestCase {
  double value;
  std::string expected;
};

class DoubleTest : public testing::TestWithParam<DoubleTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    DoubleTest, DoubleTest,
    testing::ValuesIn(std::vector<DoubleTestCase>{
        {0, std::string(8, '\0')},
        {-1, std::string("\0\0\0\0\0\0\360\277", 8)},
        {1, std::string("\0\0\0\0\0\0\360\077", 8)},
        {1.5, std::string("\0\0\0\0\0\0\370\077", 8)},
        {1.123456, "\154\353\247\377\254\371\361\077"},
    }),
    [](const testing::TestParamInfo<DoubleTest::ParamType>& info) {
      return absl::StrReplaceAll(absl::StrCat(info.param.value),
                                 {{".", "_"}, {"-", "neg"}});
    });

TEST_P(DoubleTest, Check) {
  auto& param = GetParam();
  EXPECT_EQ(Double(param.value), Wire(param.expected));
}

struct LengthPrefixedTestCase {
  std::string name;
  std::string value;
  std::string expected;
};

class LengthPrefixedTest
    : public testing::TestWithParam<LengthPrefixedTestCase> {};

INSTANTIATE_TEST_SUITE_P(
    LengthPrefixedTest, LengthPrefixedTest,
    testing::ValuesIn(std::vector<LengthPrefixedTestCase>{
        {"Empty", "", std::string("\0", 1)},
        {"Ascii", "abc", "\003abc"},
        {"NonAscii", "\x80\x81\x82", "\003\x80\x81\x82"},
        {"Long", std::string(128, 'a'), "\200\001" + std::string(128, 'a')},
    }),
    [](const testing::TestParamInfo<LengthPrefixedTest::ParamType>& info) {
      return info.param.name;
    });

TEST_P(LengthPrefixedTest, Check) {
  auto& param = GetParam();
  EXPECT_EQ(LengthPrefixed(param.value), Wire(param.expected));
}

TEST_F(LengthPrefixedTest, Nested) {
  EXPECT_EQ(LengthPrefixed(Wire("foo")), Wire("\003foo"));
}

TEST(PackedTest, Empty) {
  EXPECT_EQ(Packed(Varint<int>, std::vector<int>{}),
            Wire(absl::string_view("\0", 1)));
}

TEST(PackedTest, OneFixed32) {
  EXPECT_EQ(Packed(Fixed32<int>, {9}),
            Wire(absl::string_view("\001\011\000\000\000", 5)));
}

TEST(PackedTest, Varints) {
  EXPECT_EQ(Packed(Varint<int>, {9, 8}), Wire("\002\011\010"));
}

TEST(PackedTest, SInt32s) {
  EXPECT_EQ(Packed(SInt32, {0, 1, -1}),
            Wire(absl::string_view("\003\000\002\001", 4)));
}

TEST(PackedTest, Container) {
  std::vector<int> v = {9, 8};
  EXPECT_EQ(Packed(Varint<int>, v), Wire("\002\011\010"));
}

TEST(FieldTest, VarintField) { EXPECT_EQ(VarintField(9, 1), Wire("\110\001")); }

TEST(FieldTest, LongVarintField) {
  EXPECT_EQ(LongVarintField(9, 1, 1),
            Wire(absl::string_view("\110\201\000", 3)));
}

TEST(FieldTest, SInt32Field) {
  EXPECT_EQ(SInt32Field(9, -1), Wire("\110\001"));
}

TEST(FieldTest, SInt64Field) {
  EXPECT_EQ(SInt64Field(9, -1), Wire("\110\001"));
}

TEST(FieldTest, Fixed32Field) {
  EXPECT_EQ(Fixed32Field(9, 1), Wire(absl::string_view("\115\1\0\0\0", 5)));
}

TEST(FieldTest, Fixed64Field) {
  EXPECT_EQ(Fixed64Field(9, 1),
            Wire(absl::string_view("\111\1\0\0\0\0\0\0\0", 9)));
}

TEST(FieldTest, FloatField) {
  EXPECT_EQ(FloatField(9, 1), Wire(absl::string_view("\115\0\0\200\077", 5)));
}

TEST(FieldTest, DoubleField) {
  EXPECT_EQ(DoubleField(9, 1),
            Wire(absl::string_view("\111\0\0\0\0\0\0\360\077", 9)));
}

TEST(FieldTest, LengthPrefixedField) {
  EXPECT_EQ(LengthPrefixedField(9, "foo"), Wire("\112\003foo"));
}

TEST(FieldTest, LengthPrefixedFieldNested) {
  EXPECT_EQ(LengthPrefixedField(9, Wire("foo")), Wire("\112\003foo"));
}

TEST(FieldTest, DelimitedField) {
  EXPECT_EQ(DelimitedField(9, Wire("foo")), Wire("\113foo\114"));
}

TEST(FieldTest, PackedField) {
  EXPECT_EQ(PackedField(9, Varint<int>, {1, 2, 3}),
            Wire("\112\003\001\002\003"));
}

TEST(FieldTest, PackedFieldContainer) {
  std::vector<int> v = {1, 2, 3};
  EXPECT_EQ(PackedField(9, Varint<int>, v), Wire("\112\003\001\002\003"));
}

}  // namespace
}  // namespace conformance
}  // namespace protobuf
}  // namespace google
