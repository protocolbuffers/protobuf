// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "google/protobuf/io/strtod.h"

#include <cmath>
#include <limits>
#include <string>

#include <gtest/gtest.h>

namespace google {
namespace protobuf {
namespace io {
namespace {

TEST(StrtodTest, DoubleRegressionCorpus) {
  const double values[] = {
      0.0,
      -0.0,
      std::numeric_limits<double>::denorm_min(),
      std::numeric_limits<double>::min(),
      std::numeric_limits<double>::max(),
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
      std::numeric_limits<double>::quiet_NaN(),
      0x1p-1022,
      0x1p-1021,
      0x1p-1,
      0x1p52,
      0x1p53,
      0x1p1023,
      1e-5,
      1e-4,
      1e15,
      1e16,
      1.2345678901234567,
      0.8455124082255701,
      9.99999999999999e-5,
      9.999999999999999e-5,
      9.99999999999999e15,
      9.999999999999999e15,
  };

  for (double value : values) {
    const std::string text = SimpleDtoa(value);
    const double parsed = NoLocaleStrtod(text.c_str(), nullptr);
    if (std::isnan(value)) {
      EXPECT_TRUE(std::isnan(parsed)) << text;
    } else {
      EXPECT_EQ(parsed, value) << text;
    }
  }
}

TEST(StrtodTest, DoubleFormattingBoundaries) {
  EXPECT_EQ(SimpleDtoa(0.0), "0");
  EXPECT_EQ(SimpleDtoa(-0.0), "-0");
  EXPECT_EQ(SimpleDtoa(1e-4).find_first_of("eE"), std::string::npos);
  EXPECT_NE(SimpleDtoa(1e-5).find_first_of("eE"), std::string::npos);
  EXPECT_NE(SimpleDtoa(1e15).find_first_of("eE"), std::string::npos);
  EXPECT_NE(SimpleDtoa(1e16).find_first_of("eE"), std::string::npos);
  EXPECT_EQ(SimpleDtoa(std::numeric_limits<double>::infinity()), "inf");
  EXPECT_EQ(SimpleDtoa(-std::numeric_limits<double>::infinity()), "-inf");
  EXPECT_EQ(SimpleDtoa(std::numeric_limits<double>::quiet_NaN()), "nan");
}

TEST(StrtodTest, FloatHistoricalPathRoundTrips) {
  const float values[] = {
      0.0f,
      -0.0f,
      std::numeric_limits<float>::denorm_min(),
      std::numeric_limits<float>::min(),
      std::numeric_limits<float>::max(),
      1e-5f,
      1e-4f,
      1e5f,
      1e6f,
  };

  for (float value : values) {
    const std::string text = SimpleFtoa(value);
    char* end = nullptr;
    const float parsed = static_cast<float>(NoLocaleStrtod(text.c_str(), &end));
    ASSERT_NE(end, nullptr);
    EXPECT_EQ(*end, '\0') << text;
    EXPECT_EQ(parsed, value) << text;
  }
}

}  // namespace
}  // namespace io
}  // namespace protobuf
}  // namespace google
