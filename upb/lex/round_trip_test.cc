#include "upb/lex/round_trip.h"

#include <math.h>

#include <gtest/gtest.h>

namespace {

TEST(RoundTripTest, Double) {
  char buf[32];

  _upb_EncodeRoundTripDouble(0.123456789, buf, sizeof(buf));
  EXPECT_STREQ(buf, "0.123456789");

  _upb_EncodeRoundTripDouble(0.0, buf, sizeof(buf));
  EXPECT_STREQ(buf, "0");

  _upb_EncodeRoundTripDouble(nan(""), buf, sizeof(buf));
  EXPECT_STREQ(buf, "nan");
}

TEST(RoundTripTest, Float) {
  char buf[32];

  _upb_EncodeRoundTripFloat(0.123456, buf, sizeof(buf));
  EXPECT_STREQ(buf, "0.123456");

  _upb_EncodeRoundTripFloat(0.0, buf, sizeof(buf));
  EXPECT_STREQ(buf, "0");

  _upb_EncodeRoundTripFloat(nan(""), buf, sizeof(buf));
  EXPECT_STREQ(buf, "nan");
}

}  // namespace
