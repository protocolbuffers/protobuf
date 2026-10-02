// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/io/strtod.h"

#include <locale.h>

#include <string>
#include <tuple>

#include <gtest/gtest.h>
#include "absl/log/absl_log.h"

namespace google {
namespace protobuf {
namespace io {
namespace {

TEST(Strtod, ImmuneToLocales) {
  // Remember the old locale.
  char* old_locale_cstr = setlocale(LC_NUMERIC, nullptr);
  ASSERT_TRUE(old_locale_cstr != nullptr);
  std::string old_locale = old_locale_cstr;

  // Set the locale to "C".
  ASSERT_TRUE(setlocale(LC_NUMERIC, "C") != nullptr);

  EXPECT_EQ("1.5", SimpleDtoa(1.5));
  EXPECT_EQ("1.5", SimpleFtoa(1.5));

  if (setlocale(LC_NUMERIC, "es_ES") == nullptr &&
      setlocale(LC_NUMERIC, "es_ES.utf8") == nullptr) {
    // Some systems may not have the desired locale available.
    ABSL_LOG(WARNING) << "Couldn't set locale to es_ES.  Skipping this test.";
  } else {
    EXPECT_EQ("1.5", SimpleDtoa(1.5));
    EXPECT_EQ("1.5", SimpleFtoa(1.5));
  }

  // Return to original locale.
  setlocale(LC_NUMERIC, old_locale.c_str());
}

class FloatToString
    : public testing::TestWithParam<std::tuple<float, std::string>> {};

INSTANTIATE_TEST_SUITE_P(
    FloatToStringInst, FloatToString,
    testing::Values(std::make_tuple(1.0f, "1"), std::make_tuple(0.1f, "0.1"),
                    std::make_tuple(1e-6f, "1e-06"),
                    std::make_tuple(0.123456f, "0.123456"),
                    std::make_tuple(30.0001f, "30.0001"),
                    std::make_tuple(1.0 / 3.0, "0.33333334")));

TEST_P(FloatToString, Single) {
  float f = std::get<0>(GetParam());
  std::string actual = SimpleFtoa(f);
  const std::string& expected = std::get<1>(GetParam());
  // Per b/254169027, different platforms or implementations may produce
  // slightly different representations for the last bits of precision
  // (e.g. "0.33333334" vs "0.333333343" for 1/3), but both must round-trip
  // to the same float value.
  if (actual == expected) {
    EXPECT_EQ(actual, expected);
  } else {
    EXPECT_EQ(SafeDoubleToFloat(NoLocaleStrtod(actual.c_str(), nullptr)), f);
  }
  EXPECT_EQ(SafeDoubleToFloat(NoLocaleStrtod(expected.c_str(), nullptr)), f);
}

TEST_P(FloatToString, RoundTripFloat) {
  float f = std::get<0>(GetParam());
  std::string str = SimpleFtoa(f);
  EXPECT_EQ(SafeDoubleToFloat(NoLocaleStrtod(str.c_str(), nullptr)), f);
}

TEST_P(FloatToString, RoundTripString) {
  std::string str = std::get<1>(GetParam());
  float f = SafeDoubleToFloat(NoLocaleStrtod(str.c_str(), nullptr));
  std::string roundtrip_str = SimpleFtoa(f);
  // Per b/254169027, the string representation may differ in the last bit,
  // but parsing it back must yield the same float.
  if (roundtrip_str == str) {
    EXPECT_EQ(roundtrip_str, str);
  } else {
    EXPECT_EQ(SafeDoubleToFloat(NoLocaleStrtod(roundtrip_str.c_str(), nullptr)),
              f);
  }
}

class DoubleToString
    : public testing::TestWithParam<std::tuple<double, std::string>> {};

INSTANTIATE_TEST_SUITE_P(
    DoubleToStringInst, DoubleToString,
    testing::Values(std::make_tuple(1.0, "1"), std::make_tuple(0.1, "0.1"),
                    std::make_tuple(1e-6, "1e-06"),
                    std::make_tuple(0.1234567891, "0.1234567891"),
                    std::make_tuple(30.0000001, "30.0000001"),
                    std::make_tuple(1.0 / 3.0, "0.33333333333333331")));

TEST_P(DoubleToString, Single) {
  EXPECT_EQ(SimpleDtoa(std::get<0>(GetParam())), std::get<1>(GetParam()));
  EXPECT_EQ(NoLocaleStrtod(std::get<1>(GetParam()).c_str(), nullptr),
            std::get<0>(GetParam()));
}

TEST_P(DoubleToString, RoundTripDouble) {
  double d = std::get<0>(GetParam());
  std::string str = SimpleDtoa(d);
  EXPECT_EQ(NoLocaleStrtod(str.c_str(), nullptr), d);
}

TEST_P(DoubleToString, RoundTripString) {
  std::string str = std::get<1>(GetParam());
  double d = NoLocaleStrtod(str.c_str(), nullptr);
  EXPECT_EQ(SimpleDtoa(d), str);
}

}  // namespace
}  // namespace io
}  // namespace protobuf
}  // namespace google
