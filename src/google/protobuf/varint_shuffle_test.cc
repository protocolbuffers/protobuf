// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/varint_shuffle.h"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// Must be included last.
#include "google/protobuf/port_def.inc"

using testing::Eq;
using testing::IsNull;
using testing::NotNull;
using testing::Range;
using testing::TestWithParam;

namespace google {
namespace protobuf {
namespace internal {
namespace {

uint64_t ToInt64(char c) { return static_cast<uint8_t>(c); }
uint32_t ToInt32(char c) { return static_cast<uint8_t>(c); }

int32_t Shl(uint32_t v, int bits) { return static_cast<int32_t>(v << bits); }
int32_t Shl(int32_t v, int bits) { return Shl(static_cast<uint32_t>(v), bits); }

int64_t Shl(uint64_t v, int bits) { return static_cast<int64_t>(v << bits); }
int64_t Shl(int64_t v, int bits) { return Shl(static_cast<uint64_t>(v), bits); }

int NaiveParse(const char* p, int32_t& res) {
  int len = 0;
  auto r = ToInt32(*p);
  while (*p++ & 0x80) {
    if (++len == 10) return 11;
    if (len < 5) r += Shl(ToInt32(*p) - 1, len * 7);
  }
  res = r;
  return ++len;
}

// A naive, easy to verify implementation for test purposes.
int NaiveParse(const char* p, int64_t& res) {
  int len = 0;
  auto r = ToInt64(*p);
  while (*p++ & 0x80) {
    if (++len == 10) return 11;
    r += Shl(ToInt64(*p) - 1, len * 7);
  }
  res = r;
  return ++len;
}

// A naive, easy to verify implementation for test purposes.
int NaiveSerialize(char* p, uint64_t value) {
  int n = 0;
  while (value > 127) {
    p[n++] = 0x80 | static_cast<char>(value);
    value >>= 7;
  }
  p[n++] = static_cast<char>(value);
  return n;
}

class ShiftMixParseVarint32Test : public TestWithParam<int> {
 public:
  int length() const { return GetParam(); }
};
class ShiftMixParseVarint64Test : public TestWithParam<int> {
 public:
  int length() const { return GetParam(); }
};

INSTANTIATE_TEST_SUITE_P(Default, ShiftMixParseVarint32Test, Range(1, 11));
INSTANTIATE_TEST_SUITE_P(Default, ShiftMixParseVarint64Test, Range(1, 11));

template <int limit = 10>
const char* Parse(const char* data, int32_t& res) {
  int64_t res64;
  const char* ret = ShiftMixParseVarint<int32_t, limit>(data, res64);
  res = res64;
  return ret;
}

template <int limit = 10>
const char* Parse(const char* data, int64_t& res) {
  return ShiftMixParseVarint<int64_t, limit>(data, res);
}

template <int limit = 0>
const char* ParseWithLimit(int rtlimit, const char* data, int32_t& res) {
  if (rtlimit > limit) return ParseWithLimit<limit + 1>(rtlimit, data, res);
  return Parse<limit>(data, res);
}

template <int limit = 0>
const char* ParseWithLimit(int rtlimit, const char* data, int64_t& res) {
  if (rtlimit > limit) return ParseWithLimit<limit + 1>(rtlimit, data, res);
  return Parse<limit>(data, res);
}

template <>
const char* ParseWithLimit<10>(int rtlimit, const char* data, int32_t& res) {
  return Parse<10>(data, res);
}

template <>
const char* ParseWithLimit<10>(int rtlimit, const char* data, int64_t& res) {
  return Parse<10>(data, res);
}

template <typename T>
void TestAllLengths(int len) {
  std::vector<char> bytes;
  for (int i = 1; i < len; ++i) {
    bytes.push_back(static_cast<char>(0xC1 + (i << 1)));
  }
  bytes.push_back('\x01');
  const char* data = bytes.data();

  T expected;
  ASSERT_THAT(NaiveParse(data, expected), Eq(len));

  T result;
  const char* p = Parse(data, result);
  ASSERT_THAT(p, NotNull());
  ASSERT_THAT(p - data, Eq(len));
  ASSERT_THAT(result, Eq(expected));
}

TEST_P(ShiftMixParseVarint32Test, AllLengths) {
  TestAllLengths<int32_t>(length());
}

TEST_P(ShiftMixParseVarint64Test, AllLengths) {
  TestAllLengths<int64_t>(length());
}

template <typename T>
void TestNonCanonicalValue(int len) {
  char data[] = {'\xc3', '\xc5', '\xc7', '\xc9', '\xcb',
                 '\xcd', '\xcf', '\xd1', '\xd3', '\x7E'};
  if (len < 10) data[len++] = 0;

  T expected;
  ASSERT_THAT(NaiveParse(data, expected), Eq(len));

  T result;
  const char* p = Parse(data, result);
  ASSERT_THAT(p, NotNull());
  ASSERT_THAT(p - data, Eq(len));
  ASSERT_THAT(result, Eq(expected));
}

TEST_P(ShiftMixParseVarint32Test, NonCanonicalValue) {
  TestNonCanonicalValue<int32_t>(length());
}

TEST_P(ShiftMixParseVarint64Test, NonCanonicalValue) {
  TestNonCanonicalValue<int64_t>(length());
}

template <typename T>
void TestNonCanonicalZero(int len) {
  char data[] = {'\x80', '\x80', '\x80', '\x80', '\x80',
                 '\x80', '\x80', '\x80', '\x80', '\x7E'};
  if (len < 10) data[len++] = 0;

  T expected;
  ASSERT_THAT(NaiveParse(data, expected), Eq(len));
  ASSERT_THAT(expected, Eq(0));

  T result;
  const char* p = Parse(data, result);
  ASSERT_THAT(p, NotNull());
  ASSERT_THAT(p - data, Eq(len));
  ASSERT_THAT(result, Eq(expected));
}  // namespace

TEST_P(ShiftMixParseVarint32Test, NonCanonicalZero) {
  TestNonCanonicalZero<int32_t>(length());
}

TEST_P(ShiftMixParseVarint64Test, NonCanonicalZero) {
  TestNonCanonicalZero<int64_t>(length());
}

TEST_P(ShiftMixParseVarint32Test, HittingLimit) {
  const int limit = length();

  int32_t res = 0x94939291L;
  char data[10];
  int serialized_len = NaiveSerialize(data, res);
  ASSERT_THAT(serialized_len, Eq(10));

  int32_t result;
  const char* p = ParseWithLimit(limit, data, result);
  ASSERT_THAT(p, testing::NotNull());
  ASSERT_THAT(p - data, Eq(limit));
  if (limit < 5) {
    res |= Shl(int32_t{-1}, limit * 7);
  }
  ASSERT_THAT(result, Eq(res));
}

TEST_P(ShiftMixParseVarint64Test, HittingLimit) {
  const int limit = length();

  int64_t res = 0x9897969594939291LL;
  char data[10];
  int serialized_len = NaiveSerialize(data, res);
  ASSERT_THAT(serialized_len, Eq(10));

  int64_t result;
  const char* p = ParseWithLimit(limit, data, result);
  ASSERT_THAT(p, testing::NotNull());
  ASSERT_THAT(p - data, Eq(limit));
  if (limit != 10) {
    res |= Shl(int64_t{-1}, limit * 7);
  }
  ASSERT_THAT(result, Eq(res));
}

TEST_P(ShiftMixParseVarint32Test, AtOrBelowLimit) {
  const int limit = length();
  if (limit > 5) GTEST_SKIP() << "N/A";

  int32_t res = 0x94939291ULL >> (35 - 7 * limit);
  char data[10];
  int serialized_len = NaiveSerialize(data, res);
  ASSERT_THAT(serialized_len, Eq(limit == 5 ? 10 : limit));

  int32_t result;
  const char* p = ParseWithLimit(limit, data, result);
  ASSERT_THAT(p, testing::NotNull());
  ASSERT_THAT(p - data, Eq(limit));
  ASSERT_THAT(result, Eq(res));
}

TEST_P(ShiftMixParseVarint64Test, AtOrBelowLimit) {
  const int limit = length();

  int64_t res = 0x9897969594939291ULL >> (70 - 7 * limit);
  char data[10];
  int serialized_len = NaiveSerialize(data, res);
  ASSERT_THAT(serialized_len, Eq(limit));

  int64_t result;
  const char* p = ParseWithLimit(limit, data, result);
  ASSERT_THAT(p, testing::NotNull());
  ASSERT_THAT(p - data, Eq(limit));
  ASSERT_THAT(result, Eq(res));
}

TEST(ShiftMixParseVarint32Test, OverLong) {
  char data[] = {'\xc3', '\xc5', '\xc7', '\xc9', '\xcb',
                 '\xcd', '\xcf', '\xd1', '\xd3', '\x81'};
  int32_t result;
  const char* p = Parse(data, result);
  ASSERT_THAT(p, IsNull());
}

TEST(ShiftMixParseVarint64Test, OverLong) {
  char data[] = {'\xc3', '\xc5', '\xc7', '\xc9', '\xcb',
                 '\xcd', '\xcf', '\xd1', '\xd3', '\x81'};
  int64_t result;
  const char* p = Parse(data, result);
  ASSERT_THAT(p, IsNull());
}

TEST(ShiftMixParseVarint32Test, DroppingOverlongBits) {
  char data[] = {'\xc3', '\xc5', '\xc7', '\xc9', '\x7F'};
  int32_t expected;
  ASSERT_THAT(NaiveParse(data, expected), Eq(5));

  int32_t result;
  const char* p = Parse(data, result);
  ASSERT_THAT(p, NotNull());
  ASSERT_THAT(p - data, Eq(5));
  ASSERT_THAT(result, Eq(expected));
}

TEST(ShiftMixParseVarint64Test, DroppingOverlongBits) {
  char data[] = {'\xc3', '\xc5', '\xc7', '\xc9', '\xcb',
                 '\xcd', '\xcf', '\xd1', '\xd3', '\x7F'};
  int64_t expected;
  ASSERT_THAT(NaiveParse(data, expected), Eq(10));

  int64_t result;
  const char* p = Parse(data, result);
  ASSERT_THAT(p, NotNull());
  ASSERT_THAT(p - data, Eq(10));
  ASSERT_THAT(result, Eq(expected));
}


}  // namespace
}  // namespace internal
}  // namespace protobuf
}  // namespace google
